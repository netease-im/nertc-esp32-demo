#ifndef _ALARM_CLOCK_H_
#define _ALARM_CLOCK_H_
// #include "iot/thing.h"
#include <esp_log.h>

#include <string>
#include <vector>
#include "time.h"
#include <mutex>
#include "settings.h"
#include <esp_timer.h>

#include "lvgl.h"

#define ALARM_TAG "Alarm"

// LV_FONT_DECLARE(font_noto_thin_16_1_no_korean);

struct Alarm {
    std::string name;
    int time;
};

class AlarmManager {
public:
    AlarmManager(){
        ESP_LOGI(ALARM_TAG, "AlarmManager init");
        ring_flag = false;
        running_flag = false;

        // 建立一个时钟
        esp_timer_create_args_t timer_args = {
            .callback = [](void* arg) {
                AlarmManager* alarm_manager = (AlarmManager*)arg;
                alarm_manager->OnAlarm(); // 闹钟响了
            },
            .arg = this,
            .dispatch_method = ESP_TIMER_TASK,
            .name = "alarm_timer"
        };
        esp_timer_create(&timer_args, &timer_);
        time_t now = time(NULL);

        ClearOverdueAlarm(now);

        Alarm *current_alarm_ = GetProximateAlarm(now);
        // 启动闹钟
        if(current_alarm_ != nullptr){
            int new_timer_time = current_alarm_->time - now;
            ESP_LOGI(ALARM_TAG, "begin a alarm at %d", new_timer_time);
            esp_timer_start_once(timer_, new_timer_time * 1000000);
            running_flag = true;
        }
    }
    ~AlarmManager(){
        if(timer_ != nullptr){
            esp_timer_stop(timer_);
            esp_timer_delete(timer_);
        }
    }

    // 设置闹钟
    void SetAlarm(int seconde_from_now, std::string alarm_name){
        std::lock_guard<std::mutex> lock(mutex_);
        if(alarms_.size() >= 10){
            ESP_LOGE(ALARM_TAG, "Too many alarms");
            return;
        }
        if(seconde_from_now <= 0){
            ESP_LOGE(ALARM_TAG, "Invalid alarm time");
            return;
        }

        Settings settings_("alarm_clock", true); // 闹钟设置
        Alarm alarm; // 一个新的闹钟
        alarm.name = alarm_name;
        time_t now = time(NULL);
        alarm.time = now + seconde_from_now;
        alarms_.push_back(alarm);

        Alarm *alarm_first = GetProximateAlarm(now);
        ESP_LOGI(ALARM_TAG, "Alarm %s set at %d, now first %d", alarm.name.c_str(), alarm.time, alarm_first->time);
        if(running_flag == true){
            esp_timer_stop(timer_);
        }

        running_flag = true;

        seconde_from_now = alarm_first->time - now;
        ESP_LOGI(ALARM_TAG, "begin a alarm at %d", seconde_from_now);
        esp_timer_start_once(timer_, seconde_from_now * 1000000); // 当前一定有时钟, 所以不需要清除标志
    }
    // 获取闹钟列表状态
    std::string GetAlarmsStatus(){
        std::lock_guard<std::mutex> lock(mutex_);
        std::string status;
        for(size_t i = 0; i < alarms_.size(); ++i){
            status += alarms_[i].name + " at " + std::to_string(alarms_[i].time);
            if(i != alarms_.size() - 1){
                status += ", ";
            }
        }
        return status;
    }

    void ClearAll(){
        std::lock_guard<std::mutex> lock(mutex_);
        alarms_.clear();
    }

    void OnAlarm(std::function<void(int time, const std::string& name)> callback){
        callback_ = callback;
    }

private:
    // 清除过时的闹钟
    void ClearOverdueAlarm(time_t now){
        std::lock_guard<std::mutex> lock(mutex_);
        Settings settings_("alarm_clock", true); // 闹钟设置(硬盘存储)
        for(auto it = alarms_.begin(); it != alarms_.end();){
            if(it->time <= now){
                it = alarms_.erase(it); // 删除过期的闹钟, 此时it指向下一个元素
            }else{
                it++;
            }
        }
    }

    // 获取从现在开始第一个响的闹钟
    Alarm *GetProximateAlarm(time_t now){
        Alarm *current_alarm_ = nullptr;
        for(auto& alarm : alarms_){
            if(alarm.time > now && (current_alarm_ == nullptr || alarm.time < current_alarm_->time)){
                current_alarm_ = &alarm; // 获取当前时间以后第一个发生的时钟句柄
            }
        }
        return current_alarm_;
    }
    // 闹钟响了的处理函数
    void OnAlarm(){
        ring_flag = true;
    
        // auto display = Board::GetInstance().GetDisplay();
        // // 遍历闹钟
        Alarm *alarm_first = nullptr;
        for(auto& alarm : alarms_){
            if(alarm.time <= time(NULL)){
                alarm_first = &alarm;
                break;
            }
        }
        // display->SetStatus(alarm_first->name.c_str());  // 显示闹钟名字

        callback_(alarm_first->time, alarm_first->name);

        // // 闹钟响了
        time_t now = time(NULL);
        // 处理一下相同时间的闹钟
        ClearOverdueAlarm(now);

        Alarm *current_alarm_ = GetProximateAlarm(now);
        if(current_alarm_ != nullptr){
            int new_timer_time = current_alarm_->time - now;
            ESP_LOGI(ALARM_TAG, "begin a alarm at %d", new_timer_time);
            esp_timer_start_once(timer_, new_timer_time * 1000000);
        }else{
            running_flag = false; // 没有闹钟了
            ESP_LOGI(ALARM_TAG, "no alarm now");
        }
    }

    // 闹钟是不是响了的标志位
    bool IsRing(){ return ring_flag; };
    // 清除闹钟标志位
    void ClearRing(){ESP_LOGI("Alarm", "clear");ring_flag = false;};

    std::vector<Alarm> alarms_; // 闹钟列表
    std::mutex mutex_; // 互斥锁
    esp_timer_handle_t timer_; // 定时器

    std::atomic<bool> ring_flag{false}; 
    std::atomic<bool> running_flag{false};

    std::function<void(int time, const std::string& name)> callback_;
};

// namespace iot {

// // 这里仅定义 AlarmIot 的属性和方法，不包含具体的实现
// class Alarm : public Thing {
// private:
//     AlarmManager* alarm_manager;
//     std::function<void(int time, const std::string& name)> on_alarm_;
//     lv_obj_t* alarm_popup_ = nullptr;
//     lv_obj_t* alarm_label_ = nullptr;
//     lv_timer_t* alarm_play_timer = nullptr;
//     int alarm_cnt = -1;

// public:
//     Alarm() : Thing("Alarm", "一个闹钟, 可以定时提醒") {

//         alarm_manager = new AlarmManager();

//         alarm_popup_ = lv_obj_create(lv_screen_active());
//         lv_obj_set_scrollbar_mode(alarm_popup_, LV_SCROLLBAR_MODE_OFF);
//         lv_obj_set_size(alarm_popup_, LV_HOR_RES * 0.9, font_noto_thin_16_1_no_korean.line_height * 5);
//         // lv_obj_align(alarm_popup_, LV_ALIGN_BOTTOM_MID, 0, 0);
//         lv_obj_center(alarm_popup_);
//         lv_obj_set_style_bg_color(alarm_popup_, lv_color_black(), 0);
//         lv_obj_set_style_radius(alarm_popup_, 10, 0);
//         alarm_label_ = lv_label_create(alarm_popup_);
//         lv_label_set_text(alarm_label_, "");
//         lv_obj_set_style_text_color(alarm_label_, lv_color_white(), 0);
//         lv_obj_center(alarm_label_);

//         lv_obj_add_flag(alarm_popup_, LV_OBJ_FLAG_HIDDEN);

//         alarm_play_timer = lv_timer_create([](lv_timer_t* timer) {
//             Alarm* alarm = static_cast<Alarm*>(lv_timer_get_user_data(timer));
//             ESP_LOGW(ALARM_TAG, "lv_timer_cb: %d", alarm->alarm_cnt);
//             auto& app = Application::GetInstance();
//             alarm->alarm_cnt++;
//             if(app.GetDeviceState() == kDeviceStateIdle && alarm->alarm_cnt <= 5){
//                 auto codec = Board::GetInstance().GetAudioCodec();
//                 codec->EnableOutput(true);
//                 app.PlaySound(Lang::Sounds::P3_ALARM_RING);
//             }else{
//                 alarm->StopAlarm();
//             }
//         }, 4500, this);
//         lv_timer_pause(alarm_play_timer);

//         alarm_manager->OnAlarm([this](int time, const std::string& name){
//             ESP_LOGI(ALARM_TAG, "OnAlarm: %d, alarm_name: %s", time, name.c_str());
//             if(on_alarm_){
//                 on_alarm_(time, name);
//             }

//             auto& app = Application::GetInstance();
//             app.Close();
            
//             app.Schedule([this, name]() {
//                 auto& app = Application::GetInstance();
//                 if(app.GetDeviceState() == kDeviceStateIdle){
//                     StartAlarm(name);
//                 }else{
//                     app.Schedule([this, name]() {
//                         auto& app = Application::GetInstance();
//                         if(app.GetDeviceState() == kDeviceStateIdle){
//                             StartAlarm(name);
//                         }
//                     });
//                 }
//             });
//         });

//         // 定义设备的属性
//         properties_.AddStringProperty("Alarm_List", "当前闹钟的描述", [this]() -> std::string {
//             if(alarm_manager == nullptr){
//                 return std::string("AlarmManager is nullptr");
//             }
//             return alarm_manager->GetAlarmsStatus();
//         });

//         // 定义设备可以被远程执行的指令
//         methods_.AddMethod("SetAlarm", "设置一个闹钟", ParameterList({
//             Parameter("seconde_from_now", "闹钟多少秒以后响", kValueTypeNumber, true),
//             Parameter("alarm_name", "时钟的描述(名字)", kValueTypeString, true)
//         }), [this](const ParameterList& parameters) {
//             if(alarm_manager == nullptr){
//                 ESP_LOGE(ALARM_TAG, "AlarmManager is nullptr");
//                 return;
//             }
//             ESP_LOGI(ALARM_TAG, "SetAlarm");
//             int seconde_from_now = parameters["seconde_from_now"].number();
//             std::string alarm_name = parameters["alarm_name"].string();
//             // ESP_LOGI(ALARM_TAG, "seconde_from_now: %d, alarm_name: %s", seconde_from_now, alarm_name.c_str());
//             alarm_manager->SetAlarm(seconde_from_now, alarm_name);
//         });
//     }

//     void OnAlarm(std::function<void(int time, const std::string& name)> callback){
//         on_alarm_ = callback;
//     }

//     bool IsAlarm(void){
//         return (alarm_cnt > -1);
//     }

//     void StartAlarm(const std::string& name){
//         ESP_LOGW(ALARM_TAG, "StartAlarm");
//         alarm_cnt = 0;
//         auto display = Board::GetInstance().GetDisplay();
//         DisplayLockGuard lock(display);
//         lv_label_set_text(this->alarm_label_, name.c_str());
//         lv_obj_remove_flag(this->alarm_popup_, LV_OBJ_FLAG_HIDDEN);
//         display->SetStatus("提醒");
//         if(lv_timer_get_paused(this->alarm_play_timer)){
//             lv_timer_ready(this->alarm_play_timer);
//             lv_timer_resume(this->alarm_play_timer);
//         }
//     }

//     void StopAlarm(void){
//         ESP_LOGW(ALARM_TAG, "StopAlarm");
//         if(alarm_cnt > -1){
//             alarm_cnt = -1;
//             DisplayLockGuard lock(Board::GetInstance().GetDisplay());
//             lv_timer_pause(alarm_play_timer);
//             lv_obj_add_flag(alarm_popup_, LV_OBJ_FLAG_HIDDEN);
//         }
//     }
// };

// } // namespace iot

// // DECLARE_THING(Alarm);

#endif // _ALARM_CLOCK_H_