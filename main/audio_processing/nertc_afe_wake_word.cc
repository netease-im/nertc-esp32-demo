#include "nertc_afe_wake_word.h"
#include "application.h"

#include <esp_log.h>
#include <model_path.h>
#include <arpa/inet.h>
#include <sstream>
#define DETECTION_RUNNING_EVENT 1
#define DETECTION_STOP_EVENT 2
#include "protocols/nertc_protocol.h"

#define TAG "NertcAfeWakeWord"

void on_wake_word_detected_handle(const nertc_wakeup_sdk_callback_context_t* ctx, const char *wake_word)
{
    auto* multinet = static_cast<NertcAfeWakeWord*>(ctx->user_data);
    //ESP_LOGI("test", "wake_word:%s", wake_word);
    std::string call_back_wake_word = std::string(wake_word);
    multinet->DoCallBack(call_back_wake_word);
}

http_handle create_http() {
    auto network = Board::GetInstance().GetNetwork();
    auto http_unique = network->CreateHttp(0);
    Http* http = http_unique.release();

    return static_cast<void*>(http);
}

void destroy_http(http_handle handle) {
    if (!handle)
        return;

    Http* http = static_cast<Http*>(handle);
    delete http;
}

void set_http_header(http_handle handle, const char* key, const char* value) {
    if (!handle)
        return;
        
    Http* http = static_cast<Http*>(handle);
    http->SetHeader(key, value);
}

bool open_http(http_handle handle, const char* method, const char* url, const char* content, size_t length) {
    if (!handle)
        return false;
        
    Http* http = static_cast<Http*>(handle);
    http->SetContent(std::string(content, length));
    if (!http->Open(method, url)) {
        ESP_LOGE(TAG, "Failed to open HTTP connection. url: %s", url);    
        return false;
    }

    return true;
}

void close_http(http_handle handle) {
    if (!handle)
        return;

    Http* http = static_cast<Http*>(handle);
    http->Close();
}

int get_http_status_code(http_handle handle) {
    if (!handle)
        return -1;

    Http* http = static_cast<Http*>(handle);
    return http->GetStatusCode();
}

size_t get_http_body_length(http_handle handle) {
    if (!handle)
        return 0;

    Http* http = static_cast<Http*>(handle);
    return http->GetBodyLength();
}

size_t get_http_body(http_handle handle, char* buffer, size_t buffer_size) {
    if (!handle)
        return 0;

    Http* http = static_cast<Http*>(handle);
    size_t body_length = http->GetBodyLength();
    if (buffer_size < body_length) {
        ESP_LOGE(TAG, "Buffer size too small for HTTP body");
        return 0;
    }
    memcpy(buffer, http->ReadAll().c_str(), body_length);
    return body_length;
}

static nertc_wakeup_sdk_ext_http_io_t nertc_http_io = {
    .create_http = create_http,
    .destroy_http = destroy_http,
    .set_header = set_http_header,
    .open = open_http,
    .close = close_http,
    .get_status_code = get_http_status_code,
    .get_body_length = get_http_body_length,
    .get_body = get_http_body
};

NertcAfeWakeWord::NertcAfeWakeWord()
    : wake_word_pcm_(),
      wake_word_opus_() {
    event_group_ = xEventGroupCreate();
}

NertcAfeWakeWord::~NertcAfeWakeWord() {
    if (wake_word_encode_task_stack_ != nullptr) {
        heap_caps_free(wake_word_encode_task_stack_);
    }
    if(nertc_wake_word_)
    {
        nertc_wakeup_destory(nertc_wake_word_);
    }

    vEventGroupDelete(event_group_);
}

void NertcAfeWakeWord::Initialize(AudioCodec* codec) {
    if(initialized_)
    {
        return;
    }
    initialized_ = true;
    codec_ = codec;
    nertc_wakeup_sdk_event_handle_t handle =
    {
        .on_wake_word_detected = on_wake_word_detected_handle
    };
    config.event_handler = handle;

    if (Board::GetInstance().GetBoardType() == "ml307") { //4G模组，需要外部网络IO
        config.http_io = &nertc_http_io;
    } else {
        config.http_io = nullptr;
    }

    config.user_data = this;
    std::string device_id = Board::GetInstance().GetBoardName();
    config.deviceId = device_id.c_str();
    cJSON* config_json = NeRtcProtocol::ReadConfigJson();
    std::string custom_config_string;
    std::string local_config_appkey_;
    if(config_json) {
        cJSON* appkey = cJSON_GetObjectItem(config_json, "appkey");
        if (appkey && cJSON_IsString(appkey)) {
            ESP_LOGI(TAG, "local config set appkey to %s", appkey->valuestring);
            local_config_appkey_ = appkey->valuestring;
        }
        cJSON* custom_config = cJSON_GetObjectItem(config_json, "custom_config");
        if (custom_config && cJSON_IsObject(custom_config)) {
            custom_config_string = cJSON_Print(custom_config);
            ESP_LOGI(TAG, "local config set custom_config to %s", custom_config_string.c_str());
        }
        config.appkey = local_config_appkey_.c_str();
        config.custom_config = custom_config_string.c_str();
        cJSON_Delete(config_json);
    }
    nertc_wake_word_ = nertc_wakeup_create(&config);
#if defined(CONFIG_USE_AUDIO_CODEC_ENCODE_OPUS) 
    nertc_wakeup_init(nertc_wake_word_, 1, 0);
#else
    nertc_wakeup_init(nertc_wake_word_, codec_->input_channels(), codec_->input_reference());
#endif
    xTaskCreate([](void* arg) {
        auto this_ = (NertcAfeWakeWord*)arg;
        this_->task_created_ = true;
        this_->AudioDetectionTask();
        vTaskDelete(NULL);
    }, "audio_detection", 4096, this, 3, nullptr);
}

void NertcAfeWakeWord::OnWakeWordDetected(std::function<void(const std::string& wake_word)> callback) {
    wake_up_call_back_ = callback;
}

void NertcAfeWakeWord::DoCallBack(std::string& wake_word)
{
    last_detected_wake_word_ = wake_word;
    wake_up_call_back_(last_detected_wake_word_);
}

void NertcAfeWakeWord::StartDetection() {
    xEventGroupSetBits(event_group_, DETECTION_RUNNING_EVENT);
}

void NertcAfeWakeWord::StopDetection() {
    xEventGroupClearBits(event_group_, DETECTION_RUNNING_EVENT);
    if (nertc_wake_word_ != nullptr) {
        nertc_wakeup_stop_detect(nertc_wake_word_);
    }
    xEventGroupSetBits(event_group_, DETECTION_STOP_EVENT);
    destory_inner_models_ = true;
}

bool NertcAfeWakeWord::IsDetectionRunning() {
    return xEventGroupGetBits(event_group_) & DETECTION_RUNNING_EVENT;
}

void NertcAfeWakeWord::Feed(const std::vector<int16_t>& data) {
    if(nertc_wake_word_ == nullptr || !task_created_) {
        return;
    }
    StoreWakeWordData(data.data(), data.size());

    nertc_wakeup_feed(nertc_wake_word_, data.data(), data.size());
}

size_t NertcAfeWakeWord::GetFeedSize() {
    if (nertc_wake_word_ == nullptr) {
        return 0;
    }
    return (size_t)nertc_wakeup_get_feed_size(nertc_wake_word_);
}

void NertcAfeWakeWord::AudioDetectionTask() {
    auto fetch_size = nertc_wakeup_get_feed_chunk_size(nertc_wake_word_);
    auto feed_size = nertc_wakeup_get_fetch_chunk_size(nertc_wake_word_);
    ESP_LOGI(TAG, "Audio detection task started, feed size: %d fetch size: %d",
        feed_size, fetch_size);

    while (true) 
    {
        if(destory_inner_models_)
        {
            xEventGroupWaitBits(event_group_, DETECTION_STOP_EVENT, pdTRUE, pdFALSE, portMAX_DELAY);
            if(nertc_wake_word_)
            {
                nertc_wakeup_destory(nertc_wake_word_);
                nertc_wake_word_ = nullptr;
                ESP_LOGI(TAG, "Destroy inner wake word models");
            }
            destory_inner_models_ = false;
            continue;
        }
        xEventGroupWaitBits(event_group_, DETECTION_RUNNING_EVENT, pdFALSE, pdTRUE, portMAX_DELAY);
        if(!nertc_wake_word_ && initialized_)
        {
            nertc_wake_word_ = nertc_wakeup_create(&config);
#if defined(CONFIG_USE_AUDIO_CODEC_ENCODE_OPUS) 
            nertc_wakeup_init(nertc_wake_word_, 1, 0);
#else
            nertc_wakeup_init(nertc_wake_word_, codec_->input_channels(), codec_->input_reference());
#endif
        }
        if(nertc_wake_word_)
        {
            nertc_wakeup_detect(nertc_wake_word_);
        }
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

void NertcAfeWakeWord::StoreWakeWordData(const int16_t* data, size_t samples) {
    // store audio data to wake_word_pcm_
    std::lock_guard<std::mutex> lock(wake_word_pcm_mutex_);
    wake_word_pcm_.emplace_back(std::vector<int16_t>(data, data + samples));
    // keep about 2 seconds of data, detect duration is 30ms (sample_rate == 16000, chunksize == 512)
    while (wake_word_pcm_.size() > 2000 / 30) {
        wake_word_pcm_.pop_front();
    }
}

void NertcAfeWakeWord::EncodeWakeWordData() {
    wake_word_opus_.clear();
    if (wake_word_encode_task_stack_ == nullptr) {
        wake_word_encode_task_stack_ = (StackType_t*)heap_caps_malloc(4096 * 8, MALLOC_CAP_SPIRAM);
    }
    wake_word_encode_task_ = xTaskCreateStatic([](void* arg) {
        auto this_ = (NertcAfeWakeWord*)arg;
        {
            auto start_time = esp_timer_get_time();
            auto encoder = std::make_unique<OpusEncoderWrapper>(16000, 1, Application::GetInstance().OpusFrameDurationMs());
            encoder->SetComplexity(0); // 0 is the fastest

            int packets = 0;
            std::lock_guard<std::mutex> lock_(this_->wake_word_pcm_mutex_);
            for (auto& pcm: this_->wake_word_pcm_) {
                encoder->Encode(std::move(pcm), [this_](std::vector<uint8_t>&& opus) {
                    std::lock_guard<std::mutex> lock(this_->wake_word_mutex_);
                    this_->wake_word_opus_.emplace_back(std::move(opus));
                    this_->wake_word_cv_.notify_all();
                });
                packets++;
            }
            this_->wake_word_pcm_.clear();

            auto end_time = esp_timer_get_time();
            ESP_LOGI(TAG, "Encode wake word opus %d packets in %ld ms", packets, (long)((end_time - start_time) / 1000));

            std::lock_guard<std::mutex> lock(this_->wake_word_mutex_);
            this_->wake_word_opus_.push_back(std::vector<uint8_t>());
            this_->wake_word_cv_.notify_all();
        }
        vTaskDelete(NULL);
    }, "encode_detect_packets", 4096 * 8, this, 2, wake_word_encode_task_stack_, &wake_word_encode_task_buffer_);
}

bool NertcAfeWakeWord::GetWakeWordOpus(std::vector<uint8_t>& opus) {
    std::unique_lock<std::mutex> lock(wake_word_mutex_);
    wake_word_cv_.wait(lock, [this]() {
        return !wake_word_opus_.empty();
    });
    opus.swap(wake_word_opus_.front());
    wake_word_opus_.pop_front();
    return !opus.empty();
}
