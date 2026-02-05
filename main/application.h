#ifndef _APPLICATION_H_
#define _APPLICATION_H_

#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <freertos/task.h>
#include <esp_timer.h>

#include <string>
#include <mutex>
#include <deque>
#include <memory>

#include "protocol.h"
#include "ota.h"
#include "audio_service.h"
#include "device_state_event.h"
#include "music_player/music_player.h"
#if CONFIG_CONNECTION_TYPE_NERTC
#include "alarm.h"
#endif


#define MAIN_EVENT_SCHEDULE (1 << 0)
#define MAIN_EVENT_SEND_AUDIO (1 << 1)
#define MAIN_EVENT_WAKE_WORD_DETECTED (1 << 2)
#define MAIN_EVENT_VAD_CHANGE (1 << 3)
#define MAIN_EVENT_ERROR (1 << 4)
#define MAIN_EVENT_CHECK_NEW_VERSION_DONE (1 << 5)
#define MAIN_EVENT_CLOCK_TICK (1 << 6)

#define NERTC_BOARD_NAME "yunxin"

enum AecMode {
    kAecOff,
    kAecOnDeviceSide,
    kAecOnServerSide,
    kAecOnNertc,
};

class Application {
public:
    static Application& GetInstance() {
        static Application instance;
        return instance;
    }
    // 删除拷贝构造函数和赋值运算符
    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;

    void Start();
    void MainEventLoop();
    DeviceState GetDeviceState() const { return device_state_; }
    bool IsVoiceDetected() const { return audio_service_.IsVoiceDetected(); }
    void Schedule(std::function<void()> callback);
    void SetDeviceState(DeviceState state);
    void Alert(const char* status, const char* message, const char* emotion = "", const std::string_view& sound = "");
    void DismissAlert();
    void AbortSpeaking(AbortReason reason);
    void ToggleChatState();
    void StartListening();
    void StopListening();
    void Reboot();
    void WakeWordInvoke(const std::string& wake_word);
    bool UpgradeFirmware(Ota& ota, const std::string& url = "");
    bool CanEnterSleepMode();
    void SendMcpMessage(const std::string& payload);
    void SetAecMode(AecMode mode);
    AecMode GetAecMode() const { return aec_mode_; }
    void PlaySound(const std::string_view& sound);
    AudioService& GetAudioService() { return audio_service_; }
    void Close();
    static void ParseSongListFromJson(const std::string& json, std::vector<MusicInfo>& out_list, bool& play_now);

#if CONFIG_CONNECTION_TYPE_NERTC
    // codec
    int OpusFrameDurationMs();

    // touch
    void TouchActive(int value_head, int value_body);
    void Shake(float mag, float delta, bool is_strong);
    void LiftUp();

    // photo explain
    void PhotoExplain(const std::string& request, const std::string& pre_answer, bool network_image);

    // rtcall ring
    void StartRing();
    void StopRing();

    // alarm
    AlarmError SetAlarmTime(const std::string& type, const std::string& name, int target_time_s, bool override);
    bool GetAlarmList(std::vector<AlarmInfo>& out_list);
    bool CancelAlarm();
    bool StopAlarmRinging();

    // ai sleep
    void SetAISleep();

    void ReadNertcConfig();
    bool GetNertcTestMode() const { return enable_test_mode_; }
    //
    void TestDestroy();
#endif
private:
    Application();
    ~Application();

    std::mutex mutex_;
    std::deque<std::function<void()>> main_tasks_;
    std::unique_ptr<Protocol> protocol_;
    EventGroupHandle_t event_group_ = nullptr;
    esp_timer_handle_t clock_timer_handle_ = nullptr;
    volatile DeviceState device_state_ = kDeviceStateUnknown;
    std::atomic<bool> current_pedding_speaking_{false};
    ListeningMode listening_mode_ = kListeningModeAutoStop;
    AecMode aec_mode_ = kAecOff;
    std::string last_error_message_;
    AudioService audio_service_;

    bool has_server_time_ = false;
    bool aborted_ = false;
    int clock_ticks_ = 0;
    TaskHandle_t check_new_version_task_handle_ = nullptr;
    TaskHandle_t main_event_loop_task_handle_ = nullptr;

    void OnWakeWordDetected();
    void CheckNewVersion(Ota& ota);
    void CheckAssetsVersion();
    void ShowActivationCode(const std::string& code, const std::string& message);
    void SetListeningMode(ListeningMode mode);

#if CONFIG_CONNECTION_TYPE_NERTC
    // app start
    void DealAppStartEvent();

    // common timer
    void DealTimerEvent();

    // codec
    void ResetOpusParameters();
    void SetEncodeSampleRate(int sample_rate, int frame_duration);
    int opus_frame_duration_ = 20;
    int max_opus_decode_packets_size_ = 15;
    int max_opus_encode_packets_size_ = 15;

    // touch
    static void TouchRestoreTimerCb(TimerHandle_t xTimer);
    void TouchRestoreTimer(int duration);
    void TouchRestore();
    TimerHandle_t touch_timer_ = nullptr;
    bool touch_active_ = false;
    int touch_count_ = 0;

    // rtcall ring
    static void RingTimerCb(TimerHandle_t xTimer);
    TimerHandle_t ring_timer_ = nullptr;
    bool ringing_ = false;

    // alarm
    void OnAlarmCallback(const std::string& name, const std::string& format_time);
    std::unique_ptr<AlarmManager> alarm_manager_;
    esp_timer_handle_t alarm_play_timer_handle_ = nullptr;
    std::string ringing_alarm_name_;

    // ai sleep
    bool ai_sleep_ = false;
    bool enable_test_mode_ = false;
#endif
};


class TaskPriorityReset {
public:
    TaskPriorityReset(BaseType_t priority) {
        original_priority_ = uxTaskPriorityGet(NULL);
        vTaskPrioritySet(NULL, priority);
    }
    ~TaskPriorityReset() {
        vTaskPrioritySet(NULL, original_priority_);
    }

private:
    BaseType_t original_priority_;
};

#endif // _APPLICATION_H_
