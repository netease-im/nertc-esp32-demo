#ifndef MULTINET_AFE_WAKE_WORD_H
#define MULTINET_AFE_WAKE_WORD_H

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>

#include <esp_afe_sr_models.h>
#include <esp_nsn_models.h>

#include <list>
#include <string>
#include <vector>
#include <functional>
#include <mutex>
#include <condition_variable>

#include "audio_codec.h"
#include "wake_word.h"
#include "nertc_wakeup_sdk.h"

class NertcAfeWakeWord : public WakeWord {
public:
    NertcAfeWakeWord();
    virtual ~NertcAfeWakeWord();

    bool Initialize(AudioCodec* codec, srmodel_list_t* models_list);
    void Feed(const std::vector<int16_t>& data);
    void OnWakeWordDetected(std::function<void(const std::string& wake_word)> callback);
    void DoCallBack(std::string& wake_word);
    void Start();
    void Stop();
    size_t GetFeedSize();
    void EncodeWakeWordData();
    bool GetWakeWordOpus(std::vector<uint8_t>& opus);
    const std::string& GetLastDetectedWakeWord() const { return last_detected_wake_word_; }

private:
    nertc_wakeup_sdk_t nertc_wake_word_ = nullptr;
    bool task_created_ = false;
    std::function<void(const std::string& wake_word)> wake_up_call_back_;
    std::vector<std::string> wake_words_;
    EventGroupHandle_t event_group_;
    std::function<void(const std::string& wake_word)> wake_word_detected_callback_;
    AudioCodec* codec_ = nullptr;
    std::string last_detected_wake_word_;

    TaskHandle_t wake_word_encode_task_ = nullptr;
    StaticTask_t wake_word_encode_task_buffer_;
    StackType_t* wake_word_encode_task_stack_ = nullptr;
    std::mutex wake_word_pcm_mutex_;
    std::list<std::vector<int16_t>> wake_word_pcm_;
    std::list<std::vector<uint8_t>> wake_word_opus_;
    std::mutex wake_word_mutex_;
    std::condition_variable wake_word_cv_;
    std::atomic<bool> running_ = false;

    void StoreWakeWordData(const std::vector<int16_t>& data);
};

#endif
