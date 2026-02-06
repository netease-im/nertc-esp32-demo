#include "pure_afe_audio_processor.h"
#include <esp_log.h>

#define PROCESSOR_RUNNING 0x01

#define TAG "PureAfeAudioProcessor"

PureAfeAudioProcessor::PureAfeAudioProcessor()
    : afe_data_(nullptr) {
    event_group_ = xEventGroupCreate();
}

void PureAfeAudioProcessor::Initialize(AudioCodec* codec, int frame_duration_ms, srmodel_list_t* models_list) {
    if(codec->input_channels() != 1 || codec->input_reference())
    {
        ESP_LOGE(TAG, "Only support mono input with no reference audio");
        return;
    }
    codec_ = codec;
    frame_samples_ = frame_duration_ms * 16000 / 1000;

    // Pre-allocate output buffer capacity
    output_buffer_.reserve(frame_samples_); 

    std::string input_format = "MR";
    srmodel_list_t *models;
    if (models_list == nullptr) {
        models = esp_srmodel_init("model");
    } else {
        models = models_list;
    }

    char* ns_model_name = esp_srmodel_filter(models, ESP_NSNET_PREFIX, NULL);

    afe_config_t* afe_config = afe_config_init(input_format.c_str(), NULL, AFE_TYPE_VC, AFE_MODE_HIGH_PERF);
    afe_config->aec_mode = AEC_MODE_VOIP_HIGH_PERF;
    afe_config->vad_mode = VAD_MODE_0;
    afe_config->vad_min_noise_ms = 4000;

    if (ns_model_name != nullptr) {
        ESP_LOGI(TAG, "Use NS model: %s", ns_model_name);
        afe_config->ns_model_name = ns_model_name;
        afe_config->afe_ns_mode = AFE_NS_MODE_NET;
    } 
    else{
        afe_config->afe_ns_mode = AFE_NS_MODE_WEBRTC;
    }
    afe_config->ns_init = true;
    afe_config->afe_perferred_core = 1;
    afe_config->afe_perferred_priority = 1;
    afe_config->agc_init = false;
    afe_config->memory_alloc_mode = AFE_MEMORY_ALLOC_MORE_PSRAM;
#ifdef CONFIG_USE_DEVICE_AEC
    afe_config->aec_init = true;
    afe_config->vad_init = false;
#else
    afe_config->aec_init = false;
    afe_config->vad_init = true;
#endif
    afe_iface_ = esp_afe_handle_from_config(afe_config);
    afe_data_ = afe_iface_->create_from_config(afe_config);
    feed_buffer_ptr_ = new int16_t[2 * GetFeedSize()];
    xTaskCreate([](void* arg) {
        auto this_ = (PureAfeAudioProcessor*)arg;
        this_->AudioProcessorTask();
        vTaskDelete(NULL);
    }, "audio_communication", 4096, this, 3, NULL);
}

PureAfeAudioProcessor::~PureAfeAudioProcessor() {
    if (afe_data_ != nullptr) {
        afe_iface_->destroy(afe_data_);
    }
    if(feed_buffer_ptr_ != nullptr) {
        delete[] feed_buffer_ptr_;
    }
    vEventGroupDelete(event_group_);
}

size_t PureAfeAudioProcessor::GetFeedSize() {
    if (afe_data_ == nullptr) {
        return 0;
    }
    return afe_iface_->get_feed_chunksize(afe_data_) /** codec_->input_channels()*/;
}

void PureAfeAudioProcessor::Feed(std::vector<int16_t>&& data) {
    //printf("feed data size: %d, feed size: %d\n", data.size(), GetFeedSize());
    if(data.size() != GetFeedSize())
    {
        ESP_LOGE(TAG, "Feed size mismatch, expected %d, got %d", GetFeedSize(), data.size());
        return;
    }
    if (afe_data_ == nullptr) {
        return;
    }
    input_buffer_.insert(input_buffer_.end(), data.begin(), data.end());
    CheckAfeFeedCanBeDone();
}

void PureAfeAudioProcessor::Start() {
    xEventGroupSetBits(event_group_, PROCESSOR_RUNNING);
}

void PureAfeAudioProcessor::Stop() {
    xEventGroupClearBits(event_group_, PROCESSOR_RUNNING);
    if (afe_data_ != nullptr) {
        afe_iface_->reset_buffer(afe_data_);
    }
}

bool PureAfeAudioProcessor::IsRunning() {
    return xEventGroupGetBits(event_group_) & PROCESSOR_RUNNING;
}

void PureAfeAudioProcessor::OnOutput(std::function<void(std::vector<int16_t>&& data)> callback) {
    output_callback_ = callback;
}

void PureAfeAudioProcessor::OnVadStateChange(std::function<void(bool speaking)> callback) {
    vad_state_change_callback_ = callback;
}

void PureAfeAudioProcessor::AudioProcessorTask() {
    auto fetch_size = afe_iface_->get_fetch_chunksize(afe_data_);
    auto feed_size = afe_iface_->get_feed_chunksize(afe_data_);
    ESP_LOGI(TAG, "Audio communication task started, feed size: %d fetch size: %d",
        feed_size, fetch_size);

    while (true) {
        xEventGroupWaitBits(event_group_, PROCESSOR_RUNNING, pdFALSE, pdTRUE, portMAX_DELAY);
        auto res = afe_iface_->fetch_with_delay(afe_data_, portMAX_DELAY);
        if ((xEventGroupGetBits(event_group_) & PROCESSOR_RUNNING) == 0) {
            continue;
        }
        if (res == nullptr || res->ret_value == ESP_FAIL) {
            if (res != nullptr) {
                ESP_LOGI(TAG, "Error code: %d", res->ret_value);
            }
            continue;
        }
        // VAD state change
        if (vad_state_change_callback_) {
            if (res->vad_state == VAD_SPEECH && !is_speaking_) {
                is_speaking_ = true;
                vad_state_change_callback_(true);
            } else if (res->vad_state == VAD_SILENCE && is_speaking_) {
                is_speaking_ = false;
                vad_state_change_callback_(false);
            }
        }
        if (output_callback_ ) {
            size_t samples = res->data_size / sizeof(int16_t);
            
            // Add data to buffer
            output_buffer_.insert(output_buffer_.end(), res->data, res->data + samples);
            
            // Output complete frames when buffer has enough data
            while (output_buffer_.size() >= frame_samples_) {
                if (output_buffer_.size() == frame_samples_) {
                    // If buffer size equals frame size, move the entire buffer
                    output_callback_(std::move(output_buffer_));
                    output_buffer_.clear();
                    output_buffer_.reserve(frame_samples_);
                } else {
                    // If buffer size exceeds frame size, copy one frame and remove it
                    output_callback_(std::vector<int16_t>(output_buffer_.begin(), output_buffer_.begin() + frame_samples_));
                    output_buffer_.erase(output_buffer_.begin(), output_buffer_.begin() + frame_samples_);
                }
            }
        }
    }
}

void PureAfeAudioProcessor::EnableDeviceAec(bool enable) {
    if (enable) {
        afe_iface_->enable_aec(afe_data_);
    } else {
        afe_iface_->disable_aec(afe_data_);
    }
}

void PureAfeAudioProcessor::InputReferenceAudio(const std::vector<int16_t>& data) {
    if (afe_data_ == nullptr) {
        return;
    }
    //printf("input reference audio size: %d, feed size: %d\n", data.size(), GetFeedSize());
    reference_buffer_.insert(reference_buffer_.end(), data.begin(), data.end());
    if(reference_buffer_.size() > codec_->output_sample_rate() * 2) {
        ESP_LOGW(TAG, "reference buffer overflow before feed, drop data");
        reference_buffer_.clear();
    }
}

void PureAfeAudioProcessor::CheckAfeFeedCanBeDone() {
    size_t feed_size = GetFeedSize();
    if  (input_buffer_.size() >= feed_size ) {
        int using_ref_count = reference_buffer_.size() > feed_size ? feed_size : reference_buffer_.size();
        for(int i = 0; i < feed_size; i++) {
           feed_buffer_ptr_[2 * i] = input_buffer_[i];
           if(i < using_ref_count)
           {
                feed_buffer_ptr_[2 * i + 1] = reference_buffer_[i];
           }
           else
           {
                feed_buffer_ptr_[2 * i + 1] = 0;
           }
        }
        afe_iface_->feed(afe_data_, feed_buffer_ptr_);
        input_buffer_.erase(input_buffer_.begin(), input_buffer_.begin() + feed_size);
        if(using_ref_count > 0)
        {
            reference_buffer_.erase(reference_buffer_.begin(), reference_buffer_.begin() + using_ref_count);
        }
    }
    if(input_buffer_.size() > 10 * feed_size && input_buffer_.size() > frame_samples_) {
        output_callback_(std::vector<int16_t>(input_buffer_.begin(), input_buffer_.begin() + frame_samples_));
        input_buffer_.erase(input_buffer_.begin(), input_buffer_.begin() + frame_samples_);
    }
    if(reference_buffer_.size() > 10 * feed_size) {
        ESP_LOGW(TAG, "Reference buffer overflow after feed, drop data");
        reference_buffer_.clear();
    }
}
