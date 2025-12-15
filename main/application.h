#ifndef _APPLICATION_H_
#define _APPLICATION_H_

#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <freertos/task.h>
#include <esp_timer.h>

#include <atomic>
#include <string>
#include <mutex>
#include <list>
#include <vector>
#include <condition_variable>
#include <memory>

#include <opus_encoder.h>
#include <opus_decoder.h>
#include <opus_resampler.h>

#include "protocol.h"
#include "ota.h"
#include "background_task.h"
#include "audio_processor.h"
#include "wake_word.h"
#include "audio_debugger.h"

#define SCHEDULE_EVENT (1 << 0)
#define SEND_AUDIO_EVENT (1 << 1)
#define CHECK_NEW_VERSION_DONE_EVENT (1 << 2)

enum AecMode {
    kAecOff,
    kAecOnDeviceSide,
    kAecOnServerSide,
#if defined(CONFIG_USE_NERTC_SERVER_AEC)
    kAecOnNertc
#endif
};

enum DeviceState {
    kDeviceStateUnknown,
    kDeviceStateStarting,
    kDeviceStateWifiConfiguring,
    kDeviceStateIdle,
    kDeviceStateConnecting,
    kDeviceStateListening,
    kDeviceStateSpeaking,
    kDeviceStateUpgrading,
    kDeviceStateActivating,
    kDeviceStateFatalError
};

class AlarmManager;

#ifdef CONFIG_CONNECTION_TYPE_NERTC
#define NERTC_BOARD_NAME "yunxin"
#endif
class Application {
public:
    static Application& GetInstance() {
        static Application instance;
        return instance;
    }
    // 删除拷贝构造函数和赋值运算符
    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;

    int OpusFrameDurationMs();
    void Start();
    DeviceState GetDeviceState() const { return device_state_; }
    bool IsVoiceDetected() const { return voice_detected_; }
    void Schedule(std::function<void()> callback);
    void SetDeviceState(DeviceState state);
    void Alert(const char* status, const char* message, const char* emotion = "", const std::string_view& sound = "");
    void DismissAlert();
    void AbortSpeaking(AbortReason reason);
    void ToggleChatState();
    void TakePhoto(const std::string& request);
    void SendMcpNetworkImage();
    void StartListening();
    void StopListening();
    void UpdateIotStates();
    void Reboot();
    void WakeWordInvoke(const std::string& wake_word);
    void PlaySound(const std::string_view& sound);
    bool CanEnterSleepMode();
    void SendMcpMessage(const std::string& payload);
    void SetAecMode(AecMode mode);
    AecMode GetAecMode() const { return aec_mode_; }
    BackgroundTask* GetBackgroundTask() const { return background_task_; }
    void TouchActive(int value_head, int value_body);
    void Shake(float mag, float delta, bool is_strong);
    void LiftUp();
    void StartRing();
    void StopRing();
    void SetAlarmTime(int target_time_s, const std::string& name);
    void CancelAlarm();
    bool IsAlarmActive() const { return alarm_active_; }

private:
    Application();
    ~Application();
    void ResetOpusParameters();

    std::unique_ptr<WakeWord> wake_word_;
    std::unique_ptr<AudioProcessor> audio_processor_;
    std::unique_ptr<AudioDebugger> audio_debugger_;
    Ota ota_;
#if defined(CONFIG_CONNECTION_TYPE_NERTC)
    std::mutex main_mutex_;
    std::mutex send_queue_mutex_;
#endif
    std::mutex mutex_;
    std::list<std::function<void()>> main_tasks_;
    std::unique_ptr<Protocol> protocol_;
    EventGroupHandle_t event_group_ = nullptr;
    esp_timer_handle_t clock_timer_handle_ = nullptr;
    volatile DeviceState device_state_ = kDeviceStateUnknown;
    bool touch_active_ = false;
    ListeningMode listening_mode_ = kListeningModeAutoStop;
    AecMode aec_mode_ = kAecOff;

    bool aborted_ = false;
    bool voice_detected_ = false;
    bool busy_decoding_audio_ = false;
    bool sound_play_adding_ = false;
    int clock_ticks_ = 0;
    TaskHandle_t check_new_version_task_handle_ = nullptr;

    // Audio encode / decode
    TaskHandle_t audio_loop_task_handle_ = nullptr;
    BackgroundTask* background_task_ = nullptr;
    std::chrono::steady_clock::time_point last_output_time_;
    std::list<AudioStreamPacket> audio_send_queue_;
    std::list<AudioStreamPacket> audio_decode_queue_;
    std::condition_variable audio_decode_cv_;

    // 新增：用于维护音频包的timestamp队列
    std::list<uint32_t> timestamp_queue_;
    std::mutex timestamp_mutex_;

    std::unique_ptr<OpusEncoderWrapper> opus_encoder_;
    std::unique_ptr<OpusDecoderWrapper> opus_decoder_;
#ifdef CONFIG_USE_AUDIO_CODEC_ENCODE_OPUS
    std::unique_ptr<OpusDecoderWrapper> opus_decoder2_;
    std::vector<int16_t> wake_pcm_;
    std::atomic<int> wake_task_count_ = 0;
#endif
    int opus_frame_duration_ = 20;
    int max_opus_decode_packets_size_ = 15;
    int max_opus_encode_packets_size_ = 15;

    OpusResampler input_resampler_;
    OpusResampler reference_resampler_;
    OpusResampler output_resampler_;

#if defined(CONFIG_CONNECTION_TYPE_NERTC)
    OpusResampler output_reference_resampler_;
    std::atomic<int> nertc_audio_output_task_count_ = 0;
#endif
    void MainEventLoop();
    void OnAudioInput();
#ifdef CONFIG_USE_AUDIO_CODEC_ENCODE_OPUS
    void OnAudioInputDecodeForWakeWord();
#endif
    void OnAudioOutput();
#ifdef CONFIG_CONNECTION_TYPE_NERTC
    void OnNertcAudioOutput(AudioStreamPacket&& packet);
#endif
    bool ReadAudio(std::vector<int16_t>& data, int sample_rate, int samples);
#ifdef CONFIG_USE_AUDIO_CODEC_ENCODE_OPUS
    bool ReadAudioEncoded(std::vector<uint8_t> &opus, int sample_rate, int samples);
#endif
    void WriteAudio(std::vector<int16_t> &data, int sample_rate);
#ifdef CONFIG_USE_AUDIO_CODEC_DECODE_OPUS
    void WriteAudioEncoded(std::vector<uint8_t> &opus, int sample_rate);
#endif
    void ResetDecoder();
    void SetDecodeSampleRate(int sample_rate, int frame_duration);
#if defined(CONFIG_CONNECTION_TYPE_NERTC) && defined(CONFIG_USE_NERTC_SERVER_AEC)
    void SetEncodeSampleRate(int sample_rate, int frame_duration);
#endif
    void CheckNewVersion();
    void ShowActivationCode();
    void OnClockTimer();
    void SetListeningMode(ListeningMode mode);
    void AudioLoop();
    void StartAlarmRing();

    int touch_count_ = 0;
    bool ai_sleep_ = false;
    char *buffer_ = nullptr;
    
    void TouchRestoreTimer(int duration);
    static void TouchRestoreTimerCb(TimerHandle_t xTimer);
    void TouchRestore();
    TimerHandle_t touch_timer_ = nullptr;
    static void RingTimerCb(TimerHandle_t xTimer);
    TimerHandle_t ring_timer_ = nullptr;
    bool ringing_ = false;

    static void AlarmPlayTimerCb(TimerHandle_t xTimer);
    TimerHandle_t alarm_play_timer_ = nullptr;
    std::atomic<bool> alarm_active_{false};
    std::string alarm_name_;

    void OnAlarm(int time, const std::string& name);
    std::unique_ptr<AlarmManager> alarm_manager_;
};

#endif // _APPLICATION_H_
