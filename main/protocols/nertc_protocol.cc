#include <string>
#include <cstring>
#include "nertc_protocol.h"
#include "board.h"
#include "system_info.h"
#include <esp_random.h>
#include <esp_log.h>
#include <application.h>

#if NERTC_ENABLE_CONFIG_FILE
#include <esp_spiffs.h>
#include <sys/stat.h>
#include <errno.h>
#include <dirent.h>
#include <fstream>
#endif

#define TAG "NeRtcProtocol"

#define UID 6669
#define USE_SAFE_MODE 0
#define JOIN_EVENT (1 << 0)

#define NERTC_DEFAULT_TEST_LICENSE "eyJhbGdvcml0aG0iOiJkZWZhdWx0IiwiY3JlZGVudGlhbCI6eyJhY3RpdmF0ZURhdGUiOjE3NTYzODM4OTksImV4cGlyZURhdGUiOjE3NTkwNjIyOTksImxpY2Vuc2VLZXkiOiJ5dW54aW5Db21tb25UZXN0Iiwibm9uY2UiOiI5dE9OaExnUWlVQSJ9LCJzaWduYXR1cmUiOiJKSWtmVnVNSWx2VXJMUDlXK2puZlpTY3VuRWpkMnZibGhEYWpSM1I2UWF0eU0rZjhCQ0V4Z3ZkYzlFazlRMUswQm00SUhXNHRXdEswOHVEZTRQUWgxZ1FubVBJbDVIcUh0dGtSZGlZQS9KQUVLZU1LZ3luRmt6MXBVUWhtSkR6VHFpYU1qQWNkOTBBN0pIVlprNE9YcStQR2twenZrRk52VHAvQ3Q0VlNCRlRyMkY0TDFBRVJHR2xIQU95TFBORTU4TUVKeWZwWlRHYnJHeS9Ed3BMU1EzN0FnaWhzNEh4NmVlelRoZHJ4dUFRWmZKMGJUVktjSVVpTHJPRnBWZ2xVTkdBY3pKY1N3L05ZOVBvRS9yMzdaYWdid3dNUXcydXN3S1JNZGE2RDc0bjg0dHBEZkswZ285Smd2VWE1SFliLzN5U2wrZG1nWXFIa3FnQy9QcE5KM2c9PSIsInZlcnNpb24iOiIxLjAifQ=="

NeRtcProtocol::NeRtcProtocol() {
    ESP_LOGI(TAG, "Start create: Free: %u minimal: %u", heap_caps_get_free_size(MALLOC_CAP_INTERNAL), heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL));

    int local_frame_duration_config = 0;
    std::string local_license_config;
#if NERTC_ENABLE_CONFIG_FILE
    if (!MountFileSystem()) {
        ESP_LOGE(TAG, "Failed to initialize file system");
        return;
    }

    auto* config_json = ReadConfigJson();
    if(config_json) {
        cJSON* audio_config = cJSON_GetObjectItem(config_json, "audio_config");
        if (audio_config) {
            cJSON* frame_size = cJSON_GetObjectItem(audio_config, "frame_size");
            if (cJSON_IsNumber(frame_size)) {
                ESP_LOGI(TAG, "local config set frame size to %d ms", frame_size->valueint);
                local_frame_duration_config = cJSON_GetNumberValue(frame_size);
            }
        }
        cJSON* license_config = cJSON_GetObjectItem(config_json, "license_config");
        if (license_config) {
            cJSON* local_license = cJSON_GetObjectItem(license_config, "license");
            if (cJSON_IsString(local_license)) {
                local_license_config = local_license->valuestring;
                ESP_LOGI(TAG, "local license config, size: %d", local_license_config.size());
            }
        }
        cJSON_Delete(config_json);
    }
#endif

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

    ESP_LOGI(TAG, "Start create nertc sdk: Free: %u minimal: %u", heap_caps_get_free_size(MALLOC_CAP_INTERNAL), heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL));
    nertc_sdk_config_t nertc_sdk_config = { 0 };
    nertc_sdk_config.app_key = CONFIG_NERTC_APPKEY;
    nertc_sdk_config.device_id = device_id.c_str();
    nertc_sdk_config.force_unsafe_mode = true;
    //如果打开服务端aec，这3个参数会由sdk内部控制
    nertc_sdk_config.audio_config.channels = 1;
    nertc_sdk_config.audio_config.frame_duration = local_frame_duration_config > 0 ? local_frame_duration_config : OPUS_FRAME_DURATION_MS;
#if CONFIG_IDF_TARGET_ESP32C3
    nertc_sdk_config.audio_config.sample_rate = 8000;
    nertc_sdk_config.audio_config.out_sample_rate = 8000; //指定下行采样率使用8k
#else
    nertc_sdk_config.audio_config.sample_rate = 16000;
    nertc_sdk_config.audio_config.out_sample_rate = 16000; //指定下行采样率使用16k
#endif
    ESP_LOGI(TAG, "Start set nertc sdk handler: Free: %u minimal: %u", heap_caps_get_free_size(MALLOC_CAP_INTERNAL), heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL));

    nertc_sdk_config.event_handler.on_error = OnError;
    nertc_sdk_config.event_handler.on_license_expire_warning = OnLicenseExpireWarning;
    nertc_sdk_config.event_handler.on_channel_status_changed = OnChannelStatusChanged;
    nertc_sdk_config.event_handler.on_join = OnJoin;
    nertc_sdk_config.event_handler.on_disconnect = OnDisconnect;
    nertc_sdk_config.event_handler.on_user_joined = OnUserJoined;
    nertc_sdk_config.event_handler.on_user_left = OnUserLeft;
    nertc_sdk_config.event_handler.on_user_audio_start = OnUserAudioStart;
    nertc_sdk_config.event_handler.on_user_audio_stop = OnUserAudioStop;
    nertc_sdk_config.event_handler.on_asr_caption_result = OnAsrCaptionResult;
    nertc_sdk_config.event_handler.on_ai_data = OnAiData;
    nertc_sdk_config.event_handler.on_audio_encoded_data = OnAudioData;

    // optional_config
#if CONFIG_IDF_TARGET_ESP32S3
    nertc_sdk_config.optional_config.device_performance_level = NERTC_SDK_DEVICE_LEVEL_HIGH;
#elif CONFIG_IDF_TARGET_ESP32C3
    nertc_sdk_config.optional_config.device_performance_level = NERTC_SDK_DEVICE_LEVEL_LOW;
#else
    nertc_sdk_config.optional_config.device_performance_level = NERTC_SDK_DEVICE_LEVEL_NORMAL;
#endif
#if CONFIG_USE_NERTC_SERVER_AEC
    nertc_sdk_config.optional_config.enable_server_aec = true;
#else
    nertc_sdk_config.optional_config.enable_server_aec = false;
#endif
    nertc_sdk_config.optional_config.custom_config = CONFIG_NERTC_CUSTOM_SETTING;
    
    if (Board::GetInstance().GetBoardType() == "ml307") { //4G模组，需要外部网络IO
        NeRtcExternalNetwork* ext_net = NeRtcExternalNetwork::GetInstance();
        nertc_sdk_config.optional_config.net_vtable = ext_net->GetVTable();
    } else {
        nertc_sdk_config.optional_config.net_vtable = nullptr;
    }

    nertc_sdk_config.log_cfg.log_level = NERTC_SDK_LOG_INFO;
    nertc_sdk_config.licence_cfg.license = local_license_config.empty() ? NERTC_DEFAULT_TEST_LICENSE : local_license_config.c_str();
    nertc_sdk_config.user_data = this;

    engine_ = nertc_create_engine(&nertc_sdk_config);
    auto ret = nertc_init(engine_);
    if (ret != 0) {
        ESP_LOGE(TAG, "Failed to initialize NERtc SDK, error: %d", ret);
        return;
    }

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

    event_group_ = xEventGroupCreate();

    ESP_LOGI(TAG, "Create success device_id:%s rtc_sdk version:%s mem:[Free:%u Mini: %u])", device_id.c_str(), nertc_get_version(),
            heap_caps_get_free_size(MALLOC_CAP_INTERNAL), heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL));
}

NeRtcProtocol::~NeRtcProtocol() {
    ESP_LOGI(TAG, "destroy NeRtcProtocol");
    vEventGroupDelete(event_group_);

    if (close_timer_ != nullptr) {
        esp_timer_stop(close_timer_);
        esp_timer_delete(close_timer_);
    }

    if (engine_) {
        nertc_destroy_engine(engine_);
        engine_ = nullptr;
    }

#if NERTC_ENABLE_CONFIG_FILE
    UnmountFileSystem();
#endif
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
    cname_ = std::string("80") + std::to_string(random_num);
    ESP_LOGI(TAG, "Join cname = %s", cname_.c_str());
    auto ret = nertc_join(engine_, cname_.c_str(), checksum.c_str(), UID);
    if (ret != 0) {
        ESP_LOGE(TAG, "Join failed, error: %d", ret);
        return false;
    }

    xEventGroupWaitBits(event_group_, JOIN_EVENT, pdTRUE, pdFALSE, pdMS_TO_TICKS(10000)); //最长阻塞10秒

    return join_.load();
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
    if (!engine_)
        return;

    if (phone_call_suspend_) {
        ESP_LOGI(TAG, "phone call suspend, hand up it!!!");
        nertc_ai_hang_up(engine_);
        return;
    }

    int free_sram = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
    int min_free_sram = heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL);
    ESP_LOGI(TAG, "NeRtcProtocol CloseAudioChannel Free internal: %u minimal internal: %u", free_sram, min_free_sram);

    nertc_stop_ai(engine_);

    nertc_stop_asr_caption(engine_);

    if (on_audio_channel_closed_ != nullptr) {
        on_audio_channel_closed_();
    }

    audio_channel_opened_.store(false);
}

bool NeRtcProtocol::IsAudioChannelOpened() const {
    return join_.load() && audio_channel_opened_.load();
}

bool NeRtcProtocol::SendAudio(const AudioStreamPacket& packet) {
    if (!engine_ || !join_.load())
        return false;

    if(packet.pcm_payload.empty()) {
        nertc_sdk_audio_encoded_frame_t encoded_frame;
        encoded_frame.data = const_cast<unsigned char*>(packet.payload.data());
        encoded_frame.length = packet.payload.size();
        nertc_sdk_audio_config audio_config = {server_sample_rate_, 1, samples_per_channel_};
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

cJSON* NeRtcProtocol::BuildApplicationAsrProtocol(bool local_user ,const char* text) {
    cJSON* data = cJSON_CreateObject();
    if (local_user) {
        cJSON_AddStringToObject(data, "type", "stt");
        cJSON_AddStringToObject(data, "text", text);
    } else {
        std::string type    = "tts";
        std::string state   = "sentence_start";
        cJSON_AddStringToObject(data, "type", type.c_str());
        cJSON_AddStringToObject(data, "state", state.c_str());
        cJSON_AddStringToObject(data, "text", text);
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

cJSON* NeRtcProtocol::BuildApplicationXiaoZhiIotProtocol(const std::string& name, cJSON* arguments) {
    cJSON* command = cJSON_CreateObject();
    cJSON* parameters = cJSON_CreateObject();
    cJSON_AddStringToObject(command, "name", name.c_str());

    cJSON* method_item = cJSON_GetObjectItem(arguments, "method");
    if (!method_item || !cJSON_IsString(method_item)) {
        ESP_LOGE(TAG, "BuildApplicationXiaoZhiIotProtocol method invalid");
        return nullptr;
    }
    cJSON_AddStringToObject(command, "method", method_item->valuestring);

    const cJSON* child = nullptr;
    cJSON_ArrayForEach(child, arguments) {
        if (!child->string) continue;

        std::string key = child->string;
        if (key.find("method") != std::string::npos || key.find("response") != std::string::npos)
            continue;

        if (cJSON_IsString(child)) {
            cJSON_AddStringToObject(parameters, key.c_str(), child->valuestring);
            cJSON_AddItemToObject(command, "parameters", parameters);
            break;
        }
        else if (cJSON_IsNumber(child)) {
            if (child->valuedouble == static_cast<double>(child->valueint)) {
                cJSON_AddNumberToObject(parameters, key.c_str(), child->valueint);
            } else {
                cJSON_AddNumberToObject(parameters, key.c_str(), child->valuedouble);
            }
            cJSON_AddItemToObject(command, "parameters", parameters);
            break;
        }
        else if (cJSON_IsBool(child)) {
            cJSON_AddBoolToObject(parameters, key.c_str(), cJSON_IsTrue(child));
            cJSON_AddItemToObject(command, "parameters", parameters);
            break;
        }
        else {
            continue;
        }
    }

    cJSON* commands = cJSON_CreateArray();
    cJSON_AddItemToArray(commands, command);

    cJSON* state = BuildApplicationIotStateProtocol(commands);
    return state;
}

void NeRtcProtocol::OnError(const nertc_sdk_callback_context_t* ctx, nertc_sdk_error_code_e code, const char* msg) {
    ESP_LOGE(TAG, "NERtc OnError: %d, %s", code, msg);

    NeRtcProtocol* instance = static_cast<NeRtcProtocol*>(ctx->user_data);
    if (ctx->engine && instance) {
        ESP_LOGE(TAG, "NERtc OnError: leave and rejoin room");
        nertc_leave(ctx->engine);
        vTaskDelay(pdMS_TO_TICKS(1000));
        instance->Start();
    }
}

void NeRtcProtocol::OnLicenseExpireWarning(const nertc_sdk_callback_context_t* ctx, int remaining_days) {
    ESP_LOGI(TAG, "NERtc OnLicenseExpireWarning: %d days remaining", remaining_days);
}

void NeRtcProtocol::OnChannelStatusChanged(const nertc_sdk_callback_context_t* ctx, nertc_sdk_channel_state_e status, const char *msg) {
    ESP_LOGI(TAG, "NERtc OnChannelStatusChanged: %d, %s", (int)status, msg);
}

void NeRtcProtocol::OnJoin(const nertc_sdk_callback_context_t* ctx, uint64_t cid, uint64_t uid, nertc_sdk_error_code_e code, uint64_t elapsed, const nertc_sdk_recommended_config_t* recommended_config) {
    ESP_LOGI(TAG, "NERtc OnJoin: cid:%s, uid:%s, code:%d, elapsed:%s sample_rate:%d samples_per_channel:%d frame_duration:%d out_sample_rate:%d",
                std::to_string(cid).c_str(), std::to_string(uid).c_str(), code, std::to_string(elapsed).c_str(),
                recommended_config->recommended_audio_config.sample_rate, recommended_config->recommended_audio_config.samples_per_channel,
                recommended_config->recommended_audio_config.frame_duration, recommended_config->recommended_audio_config.out_sample_rate);

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
        instance->recommended_audio_config_ = recommended_config->recommended_audio_config;
    }

    xEventGroupSetBits(instance->event_group_, JOIN_EVENT);
}

void NeRtcProtocol::OnDisconnect(const nertc_sdk_callback_context_t* ctx, nertc_sdk_error_code_e code, int reason) {
    ESP_LOGI(TAG, "NERtc OnDisconnect: code=%d reason=%d", code, reason);

    NeRtcProtocol* instance = static_cast<NeRtcProtocol*>(ctx->user_data);
    if (!instance)
        return;

    instance->CloseAudioChannel();;
    ESP_LOGI(TAG, "Stop for receive OnDisconnect, please restart later");
}

void NeRtcProtocol::OnUserJoined(const nertc_sdk_callback_context_t* ctx, const nertc_sdk_user_info* user) {
    ESP_LOGI(TAG, "NERtc OnUserJoined: %s, %s type:%d", std::to_string(user->uid).c_str(), user->name, user->type);

    NeRtcProtocol* instance = static_cast<NeRtcProtocol*>(ctx->user_data);
    if (!instance)
        return;
}

void NeRtcProtocol::OnUserLeft(const nertc_sdk_callback_context_t* ctx, const nertc_sdk_user_info* user, int reason) {
    ESP_LOGI(TAG, "NERtc OnUserLeft: %s, %d", std::to_string(user->uid).c_str(), reason);

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

    // std::string type(ai_data->type, ai_data->type_len);
    // std::string data(ai_data->data, ai_data->data_len);
    const char* type_str = ai_data->type;
    size_t type_len = ai_data->type_len;
    const char* data_str = ai_data->data;
    ESP_LOGI(TAG, "NERtc OnAiData type:%s data:%s", type_str, data_str);

    NeRtcProtocol* instance = static_cast<NeRtcProtocol*>(ctx->user_data);
    if (!instance)
        return;

    if (strncmp(type_str, "event", type_len) == 0) {
        cJSON* data_json = cJSON_Parse(data_str);
        if (!data_json) {
            ESP_LOGE(TAG, "Failed to parse JSON data");
            return;
        }
        cJSON* event = cJSON_GetObjectItem(data_json, "event");
        if (!event) {
            ESP_LOGE(TAG, "event is invalid");
            cJSON_Delete(data_json);
            return;
        }

        std::string event_str = event->valuestring;
        if (event_str.find("audio.agent.speech_") == 0) {
            cJSON* state_json = instance->BuildApplicationTtsStateProtocol(event_str);
            if (instance->on_incoming_json_) instance->on_incoming_json_(state_json);
            cJSON_Delete(state_json);
            cJSON_Delete(data_json);
            return;
        }
        if (event_str.find("conversation.") == 0) {
            if (event_str == "conversation.suspend") {
                instance->phone_call_suspend_ = true;
                // cJSON* state_json = instance->BuildApplicationTtsStateProtocol("audio.agent.speech_started");
                // if (instance->on_incoming_json_) instance->on_incoming_json_(state_json);
                // cJSON_Delete(state_json);
            } else if (event_str == "conversation.resume") {
                instance->phone_call_suspend_ = false;
                // cJSON* state_json = instance->BuildApplicationTtsStateProtocol("audio.agent.speech_stopped");
                // if (instance->on_incoming_json_) instance->on_incoming_json_(state_json);
                // cJSON_Delete(state_json);
            }
            ESP_LOGI(TAG, "phone_call_suspend_:%d", instance->phone_call_suspend_);
            cJSON_Delete(data_json);
            return;
        }
    } else if (strncmp(type_str, "tool", type_len) == 0) {
        cJSON* data_json = cJSON_Parse(data_str);
        if (!data_json) {
            ESP_LOGE(TAG, "Failed to parse JSON data");
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
        } else if (name == "yunxin_make_call") {
            cJSON* arguments_json = cJSON_Parse(arguments.c_str());
            cJSON* phoneNumber_item = cJSON_GetObjectItem(arguments_json, "arg1");
            if (!phoneNumber_item || !cJSON_IsString(phoneNumber_item)) {
                ESP_LOGE(TAG, "phoneNumber is null");
                cJSON_Delete(arguments_json);
                cJSON_Delete(data_json);
                return;
            }
            cJSON* phoneName_item = cJSON_GetObjectItem(arguments_json, "arg0");
            if (!phoneName_item || !cJSON_IsString(phoneName_item)) {
                ESP_LOGE(TAG, "phoneName is null");
                cJSON_Delete(arguments_json);
                cJSON_Delete(data_json);
                return;
            }
            std::string phone_number = phoneNumber_item->valuestring;
            std::string phone_name = phoneName_item->valuestring;
            ESP_LOGI(TAG, "phone call start name:%s -- number:%s . and stop ai", phone_name.c_str(), phone_number.c_str());
        }  else { //尝试转换成到小智通用iot格式
            cJSON* arguments_json = cJSON_Parse(arguments.c_str());
            cJSON* state_json = instance->BuildApplicationXiaoZhiIotProtocol(name, arguments_json);
            if (!state_json) {
                ESP_LOGW(TAG, "build xiaozhi iot protocol failed, ignore it");
            } else {
                if (instance->on_incoming_json_) instance->on_incoming_json_(state_json);

                cJSON_Delete(state_json);
                cJSON_Delete(arguments_json);
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
        instance->on_incoming_audio_(AudioStreamPacket{.sample_rate = instance->recommended_audio_config_.out_sample_rate,
                                                        .frame_duration = instance->server_frame_duration_,
                                                        .timestamp = 0,
                                                        .payload = std::move(payload_vector),
                                                        .muted = is_mute_packet,});
    }
}

bool NeRtcProtocol::SendText(const std::string& text) {
    ESP_LOGI(TAG, "SendText: %s", text.c_str());
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
        if (phone_call_suspend_) {
            ESP_LOGI(TAG, "phone call suspend, hand up it!!!");
            nertc_ai_hang_up(engine_);
            // phone_call_suspend_ = false;
        } else {
            nertc_ai_manual_interrupt(engine_);
            cJSON* state_json = BuildApplicationTtsStateProtocol("audio.agent.speech_stopped");
            if (on_incoming_json_) on_incoming_json_(state_json);
            cJSON_Delete(state_json);
        }
    }

    cJSON_Delete(data_json);
    return true;
}

#if NERTC_ENABLE_CONFIG_FILE
bool NeRtcProtocol::MountFileSystem() {
    static bool fs_initialized = false;
    if (fs_initialized) {
        ESP_LOGD(TAG, "File system already initialized");
        return true;
    }

    ESP_LOGI(TAG, "Initializing unified file system...");
    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",
        .partition_label = "custom",    // 统一使用custom分区
        .max_files = 5,                 // 最少的文件句柄数
        .format_if_mount_failed = true
    };

    bool is_mounted = esp_spiffs_mounted(conf.partition_label);
    if (is_mounted) {
        ESP_LOGI(TAG, "Partition '%s' already mounted", conf.partition_label);
    } else {
        esp_err_t ret = esp_vfs_spiffs_register(&conf);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to mount SPIFFS: %s", esp_err_to_name(ret));
            return false;
        }
    }

    // 获取分区信息
    size_t total = 0, used = 0;
    auto ret = esp_spiffs_info(conf.partition_label, &total, &used);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
    } else {
        ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
    }

    fs_initialized = true;
    return true;
}

void NeRtcProtocol::UnmountFileSystem() {
    esp_err_t ret = esp_vfs_spiffs_unregister("custom");
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to unregister SPIFFS: %s", esp_err_to_name(ret));
    }
}
#endif

#if NERTC_ENABLE_CONFIG_FILE
cJSON* NeRtcProtocol::ReadConfigJson() {
    FILE* file = fopen(config_file_path_.c_str(), "r");
    if (!file) {
        ESP_LOGE(TAG, "Failed to open file: %s", config_file_path_.c_str());
        return nullptr;
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    rewind(file);

    if (file_size <= 0 || file_size > 4096) {
        ESP_LOGE(TAG, "Invalid file size: %ld", file_size);
        fclose(file);
        return nullptr;
    }

    char* buffer = (char*)malloc(file_size + 1);
    if (!buffer) {
        ESP_LOGE(TAG, "Failed to allocate memory for file");
        fclose(file);
        return nullptr;
    }

    size_t bytes_read = fread(buffer, 1, file_size, file);
    fclose(file);

    if (bytes_read != (size_t)file_size) {
        ESP_LOGE(TAG, "Read size mismatch");
        free(buffer);
        return nullptr;
    }

    buffer[file_size] = '\0';
    ESP_LOGI(TAG, "Config file content: %s", buffer); //这个日志要打印出来，排查问题依赖

    cJSON* json = cJSON_Parse(buffer);
    free(buffer);

    if (!json) {
        ESP_LOGE(TAG, "Failed to parse JSON: %s", cJSON_GetErrorPtr());
        return nullptr;
    }

    return json;
}
#endif
