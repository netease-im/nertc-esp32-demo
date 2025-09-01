#ifndef _NERTC_PROTOCOL_H_
#define _NERTC_PROTOCOL_H_

#include <atomic>
#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <esp_timer.h>
#include <cJSON.h>
#include "protocol.h"
#include "nertc_sdk.h"
#include "nertc_external_network.h"

#define NERTC_ENABLE_CONFIG_FILE 1

class NeRtcProtocol : public Protocol {
public:
    NeRtcProtocol();
    ~NeRtcProtocol();

    bool Start() override;
    bool OpenAudioChannel() override;
    void CloseAudioChannel() override;
    bool IsAudioChannelOpened() const override;
    bool SendAudio(const AudioStreamPacket& packet) override;
    void SendAecReferenceAudio(const AudioStreamPacket& packet) override;
private:
    void RequestChecksum(std::string& checksum);
    void ParseFunctionCall(cJSON* data, std::string& arguments, std::string& name);

    cJSON* BuildApplicationAsrProtocol(bool local_user ,const char* text);
    cJSON* BuildApplicationTtsStateProtocol(const std::string& action);
    cJSON* BuildApplicationIotVolumeProtocol(int volume);
    cJSON* BuildApplicationIotStateProtocol(cJSON* commands);
    cJSON* BuildApplicationXiaoZhiIotProtocol(const std::string& name, cJSON* arguments);

private:
    static void OnError(const nertc_sdk_callback_context_t* ctx, nertc_sdk_error_code_e code, const char* msg);

    static void OnLicenseExpireWarning(const nertc_sdk_callback_context_t* ctx, int remaining_days);

    static void OnChannelStatusChanged(const nertc_sdk_callback_context_t* ctx, nertc_sdk_channel_state_e status, const char *msg);
    static void OnJoin(const nertc_sdk_callback_context_t* ctx, uint64_t cid, uint64_t uid, nertc_sdk_error_code_e code, uint64_t elapsed, const nertc_sdk_recommended_config_t* recommended_config);
    static void OnDisconnect(const nertc_sdk_callback_context_t* ctx, nertc_sdk_error_code_e code, int reason);
    static void OnUserJoined(const nertc_sdk_callback_context_t* ctx, const nertc_sdk_user_info* user);
    static void OnUserLeft(const nertc_sdk_callback_context_t* ctx, const nertc_sdk_user_info* user, int reason);
    static void OnUserAudioStart(const nertc_sdk_callback_context_t* ctx, uint64_t uid, nertc_sdk_media_stream_e stream_type);
    static void OnUserAudioStop(const nertc_sdk_callback_context_t* ctx, uint64_t uid, nertc_sdk_media_stream_e stream_type);
    static void OnAsrCaptionResult(const nertc_sdk_callback_context_t* ctx, nertc_sdk_asr_caption_result_t* results, int result_count);
    static void OnAiData(const nertc_sdk_callback_context_t* ctx, nertc_sdk_ai_data_result_t* ai_data);
    static void OnAudioData(const nertc_sdk_callback_context_t* ctx, uint64_t uid, nertc_sdk_media_stream_e stream_type, nertc_sdk_audio_encoded_frame_t* encoded_frame, bool is_mute_packet);
private:
    EventGroupHandle_t event_group_ = nullptr;
    std::string cname_;
    nertc_sdk_engine_t engine_ { nullptr };
    std::atomic<bool> join_ {false}; 
    std::atomic<bool> audio_channel_opened_ {false}; 
    uint64_t cid_ { 0 }; 
    uint64_t uid_ { 0 };
    nertc_sdk_audio_config_t recommended_audio_config_ { 0 };
    esp_timer_handle_t asr_timer_ { nullptr };
    esp_timer_handle_t close_timer_ { nullptr };

    bool phone_call_suspend_ { false }; 
private:
    bool SendText(const std::string& text) override;

#if NERTC_ENABLE_CONFIG_FILE
    bool MountFileSystem();
    void UnmountFileSystem();

    std::string config_file_path_ = "/spiffs/config.json";
    cJSON* ReadConfigJson();
#endif
};

#endif
