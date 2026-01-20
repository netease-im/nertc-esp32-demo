// #include "iot/thing.h"
#include "application.h"
#include <esp_log.h>

#include <string>
#include <vector>
#include "time.h"
#include <mutex>
#include "settings.h"
#include <esp_timer.h>

#include "lvgl.h"

#define ALARM_TAG "Alarm"

enum AlarmError {
    ALARM_ERROR_NONE = 0,
    ALARM_ERROR_TOO_MANY_ALARMS = 1,
    ALARM_ERROR_INVALID_ALARM_TIME = 2,
    ALARM_ERROR_INVALID_ALARM_MANAGER = 3,
};

struct AlarmInfo {
    std::string name;
    std::string format_time;
};

class AlarmManager {
private:
    struct Alarm {
        std::string type;
        std::string name;
        int time;
        std::string format_time;

        std::string ToString() const {
            return "Alarm: " + type + ", name:" + name + ", time: " + std::to_string(time) + ", format_time: " + format_time;
        }
    };

public:
    AlarmManager() {
        ESP_LOGI(ALARM_TAG, "AlarmManager init");
        ring_flag_ = false;
        running_flag_ = false;

        esp_timer_create_args_t timer_args = {
            .callback = [](void* arg) {
                AlarmManager* alarm_manager = static_cast<AlarmManager*>(arg);
                alarm_manager->OnAlarm();
            },
            .arg = this,
            .dispatch_method = ESP_TIMER_TASK,
            .name = "alarm_timer"
        };
        esp_timer_create(&timer_args, &timer_);

        {
            std::lock_guard<std::mutex> lock(mutex_);
            time_t now = time(NULL);
            ClearOverdueAlarmInternal(now);
            StartNextAlarmInternal(now);
        }
    }

    ~AlarmManager() {
        if (timer_ != nullptr) {
            // 1. 停止定时器，防止新回调触发
            esp_timer_stop(timer_);
            
            // 2. 等待可能正在执行的回调完成
            {
                std::lock_guard<std::mutex> lock(mutex_);
                callback_ = nullptr;
                alarms_.clear();
            }
            
            // 3. 删除定时器
            esp_timer_delete(timer_);
            timer_ = nullptr;
        }
    }

    AlarmError SetAlarm(const std::string& type, const std::string& name, int seconds_from_now, bool override) {
        if (seconds_from_now <= 0) {
            ESP_LOGE(ALARM_TAG, "Invalid alarm time:%d", seconds_from_now);
            return ALARM_ERROR_INVALID_ALARM_TIME;
        }

        std::lock_guard<std::mutex> lock(mutex_);
        if (alarms_.size() >= 1) {
            if (override) {
                ESP_LOGW(ALARM_TAG, "Clear all alarms first, size:%d", (int)alarms_.size());
                alarms_.clear();
            } else {
                ESP_LOGE(ALARM_TAG, "Too many alarms");
                return ALARM_ERROR_TOO_MANY_ALARMS;
            }
        }

        Settings settings_("alarm_clock", true);
        Alarm alarm;
        alarm.type = type;
        alarm.name = name;
        time_t now = time(NULL);
        time_t alarm_time = now + seconds_from_now;  // 临时变量
        alarm.time = static_cast<int>(alarm_time);
        // 格式化时间
        struct tm timeinfo;
        localtime_r(&alarm_time, &timeinfo);
        char buffer[20];
        strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &timeinfo);
        alarm.format_time = buffer;
        alarms_.push_back(alarm);

        ESP_LOGI(ALARM_TAG, "Add alarm success: [%s]", alarm.ToString().c_str());

        Alarm* alarm_first = GetProximateAlarmInternal(now);
        if (alarm_first != nullptr) {
            ESP_LOGI(ALARM_TAG, "Alarm %s set at %d, now first %d", 
                     alarm.name.c_str(), alarm.time, alarm_first->time);
        }

        if (running_flag_) {
            esp_timer_stop(timer_);
            running_flag_ = false;
        }

        StartNextAlarmInternal(now);
        return ALARM_ERROR_NONE;
    }

    bool HasActiveAlarm() {
        std::lock_guard<std::mutex> lock(mutex_);
        ESP_LOGI(ALARM_TAG, "Alarms size: %d", (int)alarms_.size());
        return !alarms_.empty();
    }

    bool GetAlarmList(std::vector<AlarmInfo>& out_list) {
        std::lock_guard<std::mutex> lock(mutex_);
        out_list.clear();
        out_list.reserve(alarms_.size());
        
        for (const auto& alarm : alarms_) {
            out_list.push_back({alarm.name, alarm.format_time});
        }
        return true;
    }

    std::string GetAlarmsStatus() {
        std::lock_guard<std::mutex> lock(mutex_);
        std::string status;
        for (size_t i = 0; i < alarms_.size(); ++i) {
            status += alarms_[i].name + " at " + std::to_string(alarms_[i].time);
            if (i != alarms_.size() - 1) {
                status += ", ";
            }
        }
        return status.empty() ? "无闹钟" : status;
    }

    void ClearAll() {
        std::lock_guard<std::mutex> lock(mutex_);
        alarms_.clear();
        if (running_flag_) {
            esp_timer_stop(timer_);
            running_flag_ = false;
        }
    }

    void SetAlarmCallback(std::function<void(const std::string& name, const std::string& format_time)> callback) {
        std::lock_guard<std::mutex> lock(mutex_);
        callback_ = callback;
    }

    bool IsRing() const { 
        return ring_flag_.load(); 
    }
    
    void ClearRing() {
        ESP_LOGI(ALARM_TAG, "Clear ring flag");
        ring_flag_ = false;
    }

private:
    void ClearOverdueAlarmInternal(time_t now) {
        for (auto it = alarms_.begin(); it != alarms_.end();) {
            if (it->time <= now) {
                ESP_LOGI(ALARM_TAG, "Removing expired alarm: %s", it->name.c_str());
                it = alarms_.erase(it);
            } else {
                ++it;
            }
        }
    }

    Alarm* GetProximateAlarmInternal(time_t now) {
        Alarm* proximate_alarm = nullptr;
        for (auto& alarm : alarms_) {
            if (alarm.time > now) {
                if (proximate_alarm == nullptr || alarm.time < proximate_alarm->time) {
                    proximate_alarm = &alarm;
                }
            }
        }
        return proximate_alarm;
    }

    void StartNextAlarmInternal(time_t now) {
        Alarm* next_alarm = GetProximateAlarmInternal(now);
        if (next_alarm != nullptr) {
            int delay_seconds = next_alarm->time - now;
            if (delay_seconds > 0) {
                ESP_LOGI(ALARM_TAG, "Starting alarm timer for %d seconds", delay_seconds);
                esp_timer_start_once(timer_, static_cast<uint64_t>(delay_seconds) * 1000000ULL);
                running_flag_ = true;
            }
        } else {
            running_flag_ = false;
            ESP_LOGI(ALARM_TAG, "No pending alarms");
        }
    }


    void OnAlarm() {
        std::lock_guard<std::mutex> lock(mutex_);
    
        ring_flag_ = true;
        running_flag_ = false;
        time_t now = time(NULL);

        Alarm triggered_alarm;
        bool found = false;
        
        for (auto& alarm : alarms_) {
            if (alarm.time <= now) {
                triggered_alarm = alarm;  // 值拷贝
                found = true;
                break;
            }
        }

        if (found) {
            ESP_LOGI(ALARM_TAG, "Alarm triggered: %s", triggered_alarm.name.c_str());
            if (callback_) {
                callback_(triggered_alarm.name, triggered_alarm.format_time);
            }
        } else {
            ESP_LOGW(ALARM_TAG, "OnAlarm called but no expired alarm found");
        }

        ClearOverdueAlarmInternal(now);
        StartNextAlarmInternal(now);
    }

private:
    std::vector<Alarm> alarms_;
    std::mutex mutex_;
    esp_timer_handle_t timer_ = nullptr;

    std::atomic<bool> ring_flag_{false};
    std::atomic<bool> running_flag_{false};

    std::function<void(const std::string& name, const std::string& format_time)> callback_;
};
