#include <string>
#include <cstring>
#include "nertc_protocol.h"
#include "board.h"
#include "system_info.h"
#include <esp_random.h>
#include <esp_log.h>
#include <application.h>

#define TAG "NeRtcProtocol"

#define UID 6669
#define USE_SAFE_MODE 0

NeRtcProtocol::NeRtcProtocol() {
    cJSON* root = cJSON_Parse(Board::GetInstance().GetJson().c_str());
    if (!root) {
        ESP_LOGE(TAG, "Failed to parse JSON broad_info");
        return;
    }

    cJSON* board_item = cJSON_GetObjectItem(root, "board");
    if (!board_item) {
        ESP_LOGE(TAG, "Invalid board info");
        cJSON_Delete(root);
        return;
    }

    cJSON* board_name_item = cJSON_GetObjectItem(board_item, "nertc_board_name");
    if (!board_name_item || !cJSON_IsString(board_name_item)) {
        ESP_LOGE(TAG, "Invalid board name in Board info JSON");
        cJSON_Delete(root);
        return;
    }

    std::string device_id = board_name_item->valuestring;
    cJSON_Delete(root);

    nertc_sdk_config_t nertc_sdk_config = { 0 };
    nertc_sdk_config.app_key = CONFIG_NERTC_APPKEY;
    nertc_sdk_config.device_id = device_id.c_str();
    //如果不打开服务端aec，可以自由配置这3个参数
    nertc_sdk_config.audio_config.sample_rate = 16000; 
    nertc_sdk_config.audio_config.channels = 1;
    nertc_sdk_config.audio_config.frame_duration = OPUS_FRAME_DURATION_MS; 

    nertc_sdk_config.event_handler.on_error = OnError;
    nertc_sdk_config.event_handler.on_channel_status_changed = OnChannelStatusChanged;
    nertc_sdk_config.event_handler.on_join = OnJoin;
    nertc_sdk_config.event_handler.on_user_joined = OnUserJoined;
    nertc_sdk_config.event_handler.on_user_left = OnUserLeft;
    nertc_sdk_config.event_handler.on_user_audio_start = OnUserAudioStart;
    nertc_sdk_config.event_handler.on_user_audio_stop = OnUserAudioStop;
    nertc_sdk_config.event_handler.on_asr_caption_result = OnAsrCaptionResult;
    nertc_sdk_config.event_handler.on_ai_data = OnAiData;
    nertc_sdk_config.event_handler.on_audio_encoded_data = OnAudioData;
    nertc_sdk_config.user_data = this;

    engine_ = nertc_create_engine(&nertc_sdk_config);
    auto ret = nertc_init(engine_);
    if (ret != 0) {
        ESP_LOGE(TAG, "Failed to initialize NERtc SDK, error: %d", ret);
        return;
    }

    ESP_LOGI(TAG, "create NeRtcProtocol success device_id:%s", device_id.c_str());

    // create close ai timer
    const esp_timer_create_args_t timer_args = {
        .callback = [](void* arg) {
            NeRtcProtocol* instance = static_cast<NeRtcProtocol*>(arg);
            if (instance) {
                instance->CloseAudioChannel();
            }
        },
        .arg = this,
        .name = "CloseAudioChannel_timer"
    };
    
    esp_err_t err = esp_timer_create(&timer_args, &close_timer_);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "create CloseAudioChannel_timer fail err: %s", esp_err_to_name(err));
        return;
    }
}

NeRtcProtocol::~NeRtcProtocol() {
    ESP_LOGI(TAG, "destroy NeRtcProtocol");

    if (close_timer_ != nullptr) {
        esp_timer_stop(close_timer_);
        esp_timer_delete(close_timer_);
    }

    if (engine_) {
        nertcrtc_destory_engine(engine_);
        engine_ = nullptr;
    }
}

bool NeRtcProtocol::Start() {   
    if (!engine_)
        return false;

    join_.store(false); 

    // get checksum
    std::string checksum;
#if USE_SAFE_MODE
    RequestChecksum(checksum);
    if (checksum.empty()) {
        ESP_LOGE(TAG, "Failed to get checksum");
        return false;
    }
#endif

    // join room
    uint32_t random_num = 100000 + (esp_random() % 900000);
    cname_ = std::string("79") + std::to_string(random_num);
    ESP_LOGI(TAG, "cname = %s\n", cname_.c_str());
    auto ret = nertc_join(engine_, cname_.c_str(), checksum.c_str(), UID);
    if (ret != 0) {
        ESP_LOGE(TAG, "Join failed, error: %d", ret);
        return false;
    }

    return true;
}

bool NeRtcProtocol::OpenAudioChannel() {
    int free_sram = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
    int min_free_sram = heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL);
    ESP_LOGI(TAG, "NeRtcProtocol OpenAudioChannel Free internal: %u minimal internal: %u", free_sram, min_free_sram);
    if (!engine_ || !join_.load()) {
        ESP_LOGE(TAG, "engine_:%d join_:%d\n", engine_ ? 1 : 0, join_.load());
        return false;
    }

    auto ret = nertc_start_ai(engine_);
    if (ret != 0) {
        ESP_LOGE(TAG, "Start AI failed, error: %d", ret);
        return false;
    }

    nertc_sdk_asr_caption_config_t asr_config;
    ret = nertc_start_asr_caption(engine_, &asr_config);
    if (ret != 0) {
        ESP_LOGE(TAG, "Start ASR caption failed, error: %d", ret);
        return false;
    }

    if (on_audio_channel_opened_ != nullptr) {
        on_audio_channel_opened_();
    }

    audio_channel_opened_.store(true);
    return true;
}

void NeRtcProtocol::CloseAudioChannel() {
    audio_channel_opened_.store(false);

    if (!engine_)
        return;
    int free_sram = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
    int min_free_sram = heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL);
    ESP_LOGI(TAG, "NeRtcProtocol CloseAudioChannel Free internal: %u minimal internal: %u", free_sram, min_free_sram);
    
    nertc_stop_ai(engine_);

    nertc_stop_asr_caption(engine_);

    if (on_audio_channel_closed_ != nullptr) {
        on_audio_channel_closed_();
    }
}

bool NeRtcProtocol::IsAudioChannelOpened() const {
    return join_.load() && audio_channel_opened_.load();
}

bool NeRtcProtocol::SendAudio(const AudioStreamPacket& packet) {
    if (!engine_)
        return false;

    if(packet.pcm_payload.empty()) {
        nertc_sdk_audio_encoded_frame_t encoded_frame;
        encoded_frame.data = const_cast<unsigned char*>(packet.payload.data());
        encoded_frame.length = packet.payload.size();
        nertc_sdk_audio_config audio_config = {16000, 1, 160};
        nertc_push_audio_encoded_frame(engine_, NERTC_SDK_MEDIA_MAIN_AUDIO, audio_config, 100, &encoded_frame);
    } else {
        if (packet.sample_rate != server_sample_rate_) {
            ESP_LOGE(TAG, "SendAudio PCM sample rate mismatch: expected %d, got %d", 
                    server_sample_rate_, packet.sample_rate);
            return false;
        }

        nertc_sdk_audio_frame_t audio_frame;
        audio_frame.type = NERTC_SDK_AUDIO_PCM_16;
        audio_frame.data = const_cast<int16_t*>(packet.pcm_payload.data());
        audio_frame.length = packet.pcm_payload.size();
        nertc_push_audio_frame(engine_, NERTC_SDK_MEDIA_MAIN_AUDIO, &audio_frame);
    }

    return true;
}

void NeRtcProtocol::SendAecReferenceAudio(const AudioStreamPacket& packet) {
    if (!engine_ || !join_.load())
        return;

    if (packet.sample_rate != server_sample_rate_) {
        ESP_LOGE(TAG, "SendAecReferenceAudio PCM sample rate mismatch: expected %d, got %d", 
                server_sample_rate_, packet.sample_rate);
        return;
    }

    nertc_sdk_audio_encoded_frame_t encoded_frame;
    encoded_frame.data = const_cast<unsigned char*>(packet.payload.data());
    encoded_frame.length = packet.payload.size();

    nertc_sdk_audio_frame_t audio_frame;
    audio_frame.type = NERTC_SDK_AUDIO_PCM_16;
    audio_frame.data = const_cast<int16_t*>(packet.pcm_payload.data());
    audio_frame.length = packet.pcm_payload.size();
    nertc_push_audio_reference_frame(engine_, NERTC_SDK_MEDIA_MAIN_AUDIO, &encoded_frame, &audio_frame);
}

void NeRtcProtocol::RequestChecksum(std::string& checksum) {
    std::ostringstream post_body;
    post_body << "uid=" << UID
              << "&appkey=" << CONFIG_NERTC_APPKEY
              << "&curtime=" << time(NULL);
    std::string post_str = post_body.str();

    auto http = Board::GetInstance().CreateHttp();
    http->SetHeader("Content-Type", "application/x-www-form-urlencoded");
    http->SetContent(std::move(post_str));
    if (!http->Open("POST", "http://webtest.netease.im/nrtcproxy/demo/getChecksum.action")) {
        ESP_LOGE(TAG, "Failed to open HTTP connection");    
        return;
    }

    auto response = http->ReadAll();
    http->Close();
    delete http;    
    cJSON* root = cJSON_Parse(response.c_str());
    if (root == nullptr) {
        ESP_LOGE(TAG, "Failed to parse JSON response: %s", response.c_str());
        return;
    }

    cJSON* code_item = cJSON_GetObjectItem(root, "code");
    if (!code_item || !cJSON_IsNumber(code_item)) {
        ESP_LOGE(TAG, "Missing or invalid code in response");
        cJSON_Delete(root);
        return;
    }
    int code = code_item->valueint;
    if (code != 200) {
        cJSON* msg_item = cJSON_GetObjectItem(root, "msg");
        std::string error_msg = msg_item && cJSON_IsString(msg_item) ? 
                            msg_item->valuestring : "Unknown error";
        ESP_LOGE(TAG, "Request failed with code: %d, msg: %s", 
                code, error_msg.c_str());
        cJSON_Delete(root);
        return;
    }

    cJSON* token_item = cJSON_GetObjectItem(root, "checksum");
    if (!token_item || !cJSON_IsString(token_item)) {
        ESP_LOGE(TAG, "Missing or invalid token in response");
        cJSON_Delete(root);
        return;
    }

    checksum = token_item->valuestring;
    cJSON_Delete(root);
}

void NeRtcProtocol::ParseFunctionCall(cJSON* data, std::string& arguments, std::string& name) {
    cJSON* toolCalls = cJSON_GetObjectItem(data, "toolCalls");
    cJSON* toolCall = cJSON_GetArrayItem(toolCalls, 0);
    if (toolCall == nullptr || !cJSON_GetObjectItem(toolCall, "function")) {
        ESP_LOGE(TAG, "toolCall is null");
        return;
    }
    cJSON* function = cJSON_GetObjectItem(toolCall, "function");
    if (!cJSON_GetObjectItem(function, "name")) {
        ESP_LOGE(TAG, "name is null");
        return;
    }

    cJSON* name_item = cJSON_GetObjectItem(function, "name");
    if (!cJSON_IsString(name_item)) {
        ESP_LOGE(TAG, "arguments or name is not string");
        return;
    }
    name = name_item->valuestring;

    cJSON* arguments_item = cJSON_GetObjectItem(function, "arguments");
    if (arguments_item && cJSON_IsString(arguments_item)) {
        arguments = arguments_item->valuestring;
    }
}

cJSON* NeRtcProtocol::BuildApplicationAsrProtocol(bool local_user ,const std::string& text) {
    cJSON* data = cJSON_CreateObject();
    if (local_user) {
        cJSON_AddStringToObject(data, "type", "stt");
        cJSON_AddStringToObject(data, "text", text.c_str());
    } else {
        std::string type    = "tts";
        std::string state   = "sentence_start";
        cJSON_AddStringToObject(data, "type", type.c_str());
        cJSON_AddStringToObject(data, "state", state.c_str());
        cJSON_AddStringToObject(data, "text", text.c_str());
    }

    return data;
}

cJSON* NeRtcProtocol::BuildApplicationTtsStateProtocol(const std::string& event) {
    cJSON* data = cJSON_CreateObject();
    cJSON_AddStringToObject(data, "type", "tts");
    std::string state;
    if (event == "audio.agent.speech_started") {
        state = "start";  
    } else if (event == "audio.agent.speech_stopped") {
        state = "stop";
    } 
    cJSON_AddStringToObject(data, "state", state.c_str());

    return data;
}

cJSON* NeRtcProtocol::BuildApplicationIotVolumeProtocol(int volume) {
    cJSON* command = cJSON_CreateObject();
    cJSON_AddStringToObject(command, "name", "AudioSpeaker");
    cJSON_AddStringToObject(command, "method", "set_volume");
    cJSON* parameters = cJSON_CreateObject();
    cJSON_AddNumberToObject(parameters, "volume", volume);
    cJSON_AddItemToObject(command, "parameters", parameters);
    cJSON* commands = cJSON_CreateArray();
    cJSON_AddItemToArray(commands, command);
    return commands;
}

cJSON* NeRtcProtocol::BuildApplicationIotStateProtocol(cJSON* commands) {
    cJSON* data = cJSON_CreateObject();
    cJSON_AddStringToObject(data, "type", "iot");
    cJSON_AddItemToObject(data, "commands", commands);
    return data;
}

void NeRtcProtocol::OnError(const nertc_sdk_callback_context_t* ctx, int code, const char* msg) {
    ESP_LOGE(TAG, "NERtc OnError: %d, %s", code, msg);

    NeRtcProtocol* instance = static_cast<NeRtcProtocol*>(ctx->user_data);
    if (ctx->engine && instance) {
        ESP_LOGE(TAG, "NERtc OnError: leave and rejoin room");
        nertc_leave(ctx->engine);
        vTaskDelay(pdMS_TO_TICKS(1000));
        instance->Start();
    }
}

void NeRtcProtocol::OnChannelStatusChanged(const nertc_sdk_callback_context_t* ctx, nertc_sdk_channel_state_e status, const char *msg) {
    ESP_LOGI(TAG, "NERtc OnChannelStatusChanged: %d, %s", (int)status, msg);
}

void NeRtcProtocol::OnJoin(const nertc_sdk_callback_context_t* ctx, uint64_t cid, uint64_t uid, nertc_sdk_error_code_e code, uint64_t elapsed, const nertc_sdk_recommended_config* recommended_config) {
    ESP_LOGI(TAG, "NERtc OnJoin: cid:%s, uid:%s, code:%d, elapsed:%s sample_rate:%d samples_per_channel:%d", 
                std::to_string(cid).c_str(), std::to_string(uid).c_str(), code, std::to_string(elapsed).c_str(), recommended_config->recommended_audio_config.sample_rate, recommended_config->recommended_audio_config.samples_per_channel);

    NeRtcProtocol* instance = static_cast<NeRtcProtocol*>(ctx->user_data);
    if (ctx->engine && instance) {
        if (code != NERTC_SDK_ERR_SUCCESS) {
            ESP_LOGE(TAG, "Failed to join room, error: %d", code);
            return;
        }

        instance->join_.store(true); 
        instance->cid_ = cid;
        instance->uid_ = uid;

        instance->server_sample_rate_ = recommended_config->recommended_audio_config.sample_rate;        
        instance->server_frame_duration_ = recommended_config->recommended_audio_config.frame_duration;
        instance->samples_per_channel_ = recommended_config->recommended_audio_config.samples_per_channel;
    }
}

void NeRtcProtocol::OnUserJoined(const nertc_sdk_callback_context_t* ctx, const nertc_sdk_user_info& user) {
    ESP_LOGI(TAG, "NERtc OnUserJoined: %s, %s type:%d", std::to_string(user.uid).c_str(), user.name, user.type);

    NeRtcProtocol* instance = static_cast<NeRtcProtocol*>(ctx->user_data);
    if (!instance)
        return;
}

void NeRtcProtocol::OnUserLeft(const nertc_sdk_callback_context_t* ctx, const nertc_sdk_user_info& user, int reason) {
    ESP_LOGI(TAG, "NERtc OnUserLeft: %s, %d", std::to_string(user.uid).c_str(), reason);

    NeRtcProtocol* instance = static_cast<NeRtcProtocol*>(ctx->user_data);
    if (!instance)
        return;
}

void NeRtcProtocol::OnUserAudioStart(const nertc_sdk_callback_context_t* ctx, uint64_t uid, nertc_sdk_media_stream_e stream_type) {
    ESP_LOGI(TAG, "NERtc OnUserAudioStart: %s, %d", std::to_string(uid).c_str(), stream_type);
}

void NeRtcProtocol::OnUserAudioStop(const nertc_sdk_callback_context_t* ctx, uint64_t uid, nertc_sdk_media_stream_e stream_type) {
    ESP_LOGI(TAG, "NERtc OnUserAudioStop: %s, %d", std::to_string(uid).c_str(), stream_type);
}

void NeRtcProtocol::OnAsrCaptionResult(const nertc_sdk_callback_context_t* ctx, nertc_sdk_asr_caption_result_t* results, int result_count) {
    NeRtcProtocol* instance = static_cast<NeRtcProtocol*>(ctx->user_data);
    if (!instance)
        return;

    for (int i = 0; i < result_count; i++) {
        auto result = results[i];
        cJSON* caption_json = instance->BuildApplicationAsrProtocol(result.is_local_user, result.content);
        if (caption_json) {
            if (instance->on_incoming_json_) instance->on_incoming_json_(caption_json);
            cJSON_Delete(caption_json);
        }
    }
}

void NeRtcProtocol::OnAiData(const nertc_sdk_callback_context_t* ctx, nertc_sdk_ai_data_result_t* ai_data) {
    if (!ai_data) 
        return;

    std::string type(ai_data->type, ai_data->type_len);
    std::string data(ai_data->data, ai_data->data_len);
    ESP_LOGI(TAG, "NERtc OnAiData type:%s data:%s", type.c_str(), data.c_str());

    NeRtcProtocol* instance = static_cast<NeRtcProtocol*>(ctx->user_data);
    if (!instance) 
        return;

    if (type == "event") {
        cJSON* state_json = instance->BuildApplicationTtsStateProtocol(data);
        if (instance->on_incoming_json_) instance->on_incoming_json_(state_json);
        cJSON_Delete(state_json);
    } else if (type == "tool") {
        cJSON* data_json = cJSON_Parse(data.c_str());
        if (!data_json) {
            ESP_LOGE(TAG, "Failed to parse JSON data: %s", data.c_str());
            return;
        }

        std::string arguments;
        std::string name;
        instance->ParseFunctionCall(data_json, arguments, name);
        if (name == "xiaozhi_SetVolume") {
            cJSON* arguments_json = cJSON_Parse(arguments.c_str());
            cJSON* volume_item = cJSON_GetObjectItem(arguments_json, "volume");
            if (volume_item == nullptr || !cJSON_IsNumber(volume_item)) {
                ESP_LOGE(TAG, "volume is null");
                cJSON_Delete(arguments_json);
                cJSON_Delete(data_json);
                return;
            }
            int volume = volume_item->valueint;
            cJSON* commands = instance->BuildApplicationIotVolumeProtocol(volume);
            cJSON* state_json = instance->BuildApplicationIotStateProtocol(commands);
            if (instance->on_incoming_json_) instance->on_incoming_json_(state_json);
            
            cJSON_Delete(state_json);
            cJSON_Delete(arguments_json);
        } else if (name == "good_bye_call" || name == "Long_Silence") {
            esp_err_t err = esp_timer_start_once(instance->close_timer_, 4 * 1000 * 1000); 
            if (err != ESP_OK) {
                ESP_LOGE(TAG, "start CloseAudioChannel_timer fail err: %s", esp_err_to_name(err));
                instance->CloseAudioChannel();
                return;
            }
        }

        cJSON_Delete(data_json);
    }
}

void NeRtcProtocol::OnAudioData(const nertc_sdk_callback_context_t* ctx, uint64_t uid, nertc_sdk_media_stream_e stream_type, nertc_sdk_audio_encoded_frame_t* encoded_frame, bool is_mute_packet) {
    NeRtcProtocol* instance = static_cast<NeRtcProtocol*>(ctx->user_data);
    if (!instance)
        return; 

    std::vector<uint8_t> payload_vector;
    if(encoded_frame->data) {
        payload_vector.assign(encoded_frame->data, encoded_frame->data + encoded_frame->length);
    }

    if (instance->on_incoming_audio_ != nullptr) {
        instance->on_incoming_audio_(AudioStreamPacket{.sample_rate = instance->server_sample_rate_,
                                                        .frame_duration = instance->server_frame_duration_,
                                                        .timestamp = 0,
                                                        .muted = is_mute_packet,
                                                        .payload = std::move(payload_vector)});
    }
}

bool NeRtcProtocol::SendText(const std::string& text) {
    if (!engine_)
        return false;
    cJSON* data_json = cJSON_Parse(text.c_str());
    cJSON* type_item = cJSON_GetObjectItem(data_json, "type");
    if (type_item == nullptr || !cJSON_IsString(type_item)) {
        ESP_LOGE(TAG, "type is null");
        cJSON_Delete(data_json);
        return false;
    }
    std::string type = type_item->valuestring;
    if (type == "abort") {
        nertc_ai_manual_interrupt(engine_);
        cJSON* state_json = BuildApplicationTtsStateProtocol("audio.agent.speech_stopped");
        if (on_incoming_json_) on_incoming_json_(state_json);
        cJSON_Delete(state_json);
    }

    cJSON_Delete(data_json);
    return true;
}
