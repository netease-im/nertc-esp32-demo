#include "application.h"
#include "board.h"
#include "display.h"
#include "system_info.h"
#include "audio_codec.h"
#include "assets/lang_config.h"
#include "nertc_protocol.h"

#include "assets/lang_config.h"

#include <esp32_camera.h>
#include <mbedtls/base64.h>
#include <esp_log.h>

#define TAG "Application"

#ifdef CONFIG_DEBUG_RUNTIME_STATS
typedef struct {
    TaskStatus_t status;
    uint32_t     runtime;
    char         name[configMAX_TASK_NAME_LEN];
} TaskSnap_t;

static TaskSnap_t *snap_last = nullptr;   /* 上一帧 */
static UBaseType_t snap_cnt  = 0;         /* 上一帧任务数 */
static portMUX_TYPE spinlock = portMUX_INITIALIZER_UNLOCKED;


static UBaseType_t take_snapshot(TaskSnap_t **out, uint32_t *total_runtime)
{
    UBaseType_t cnt = uxTaskGetNumberOfTasks();
    static TaskStatus_t *raw = nullptr;
    static UBaseType_t   raw_sz = 0;
    if (cnt > raw_sz) {
        raw_sz = cnt + 5;
        raw = (TaskStatus_t *)realloc(raw, raw_sz * sizeof(TaskStatus_t));
    }

    portENTER_CRITICAL(&spinlock);
    cnt = uxTaskGetSystemState(raw, raw_sz, total_runtime);
    portEXIT_CRITICAL(&spinlock);

    static TaskSnap_t *cur = nullptr;
    static UBaseType_t cur_sz = 0;
    if (cnt > cur_sz) {
        cur_sz = cnt + 5;
        cur = (TaskSnap_t *)realloc(cur, cur_sz * sizeof(TaskSnap_t));
    }

    for (UBaseType_t i = 0; i < cnt; ++i) {
        cur[i].status = raw[i];                         /* 整个结构体复制 */
        cur[i].runtime = raw[i].ulRunTimeCounter;       /* 累计时间 */
        /* 名字本地副本，防 NULL */
        const char *src = raw[i].pcTaskName ? raw[i].pcTaskName : "<noname>";
        size_t len = strlen(src);
        if (len >= sizeof(cur[i].name)) len = sizeof(cur[i].name) - 1;
        memcpy(cur[i].name, src, len);
        cur[i].name[len] = '\0';
    }

    *out = cur;
    return cnt;
}

void print_runtime_delta(void)
{
    uint32_t total_now;
    TaskSnap_t *cur;
    UBaseType_t cnt = take_snapshot(&cur, &total_now);

    static bool first = true;
    if (first) {
        first = false;
        snap_cnt = cnt;
        snap_last = (TaskSnap_t *)malloc(cnt * sizeof(TaskSnap_t));
        memcpy(snap_last, cur, cnt * sizeof(TaskSnap_t));
        return;
    }

    /* 总增量 */
    uint32_t total_delta = 0;
    for (UBaseType_t i = 0; i < cnt; ++i) {
        for (UBaseType_t k = 0; k < snap_cnt; ++k) {
            if (cur[i].status.xHandle == snap_last[k].status.xHandle) {
                total_delta += (cur[i].runtime - snap_last[k].runtime);
                break;
            }
        }
    }

    printf("Runtime Stats (Task Name, Runtime %%) count %d:\n", (int)cnt);
    for (UBaseType_t i = 0; i < cnt; ++i) {
        for (UBaseType_t k = 0; k < snap_cnt; ++k) {
            if (cur[i].status.xHandle == snap_last[k].status.xHandle) {
                uint32_t delta = cur[i].runtime - snap_last[k].runtime;
                float percent = total_delta ? (100.0f * delta) / total_delta : 0;
                const char *name = cur[i].name;
                if (percent > 0.99f) {
                    printf("%-16s %6.1f\n", name, percent);
                } else {
                    // printf("%-16s   <1\n", name);
                }
                break;
            }
        }
    }
    printf("**********************************************\n");

    /* 更新基准 */
    if (cnt != snap_cnt) {
        snap_cnt = cnt;
        snap_last = (TaskSnap_t *)realloc(snap_last, cnt * sizeof(TaskSnap_t));
    }
    memcpy(snap_last, cur, cnt * sizeof(TaskSnap_t));
}
#endif

void Application::DealAppStartEvent() {
    alarm_manager_ = std::make_unique<AlarmManager>();
    alarm_manager_->SetAlarmCallback(std::bind(&Application::OnAlarmCallback, this, std::placeholders::_1, std::placeholders::_2));
}

void Application::DealTimerEvent() {
#ifdef CONFIG_DEBUG_RUNTIME_STATS
    if (clock_ticks_ % 10 == 0) {
        print_runtime_delta();
    }
#endif

    if (ai_sleep_ && (device_state_ == kDeviceStateIdle || device_state_ == kDeviceStateListening)) {
        Schedule([this]() {
            ESP_LOGI(TAG, "AI sleep mode, close the audio channel");
            if (protocol_) {
                protocol_->CloseAudioChannel();
            }
            ai_sleep_ = false;
        });
    }
}

void Application::PhotoExplain(const std::string& request, const std::string& pre_answer, bool network_image) {
    if (network_image) {
        Schedule([this, request]() {
            std::string img_url = "https://pics5.baidu.com/feed/5ab5c9ea15ce36d3f5f01f21f488f897e850b1b3.jpeg";
            protocol_->SendLlmImage(img_url.c_str(), img_url.size(), 0, request, 1);
        });
    } else {
        Schedule([this, request, pre_answer]() {
            if (!pre_answer.empty()) {
                protocol_->SendTTSText(pre_answer, 2, false);
            }

            Camera* camera = Board::GetInstance().GetCamera();
            if (camera) {
                camera->Capture();
                JpegChunk jpeg = {nullptr, 0};
                if (camera->GetCapturedJpeg(jpeg.data, jpeg.len)) {
                    ESP_LOGI(TAG, "Captured JPEG size: %d", jpeg.len);

                    // 检查输入参数有效性
                    if (!jpeg.data || jpeg.len == 0) {
                        ESP_LOGE(TAG, "Invalid JPEG data: data=%p, len=%d", jpeg.data, jpeg.len);
                        if (jpeg.data) {
                            heap_caps_free(jpeg.data);
                        }
                        return;
                    }

                    const char* image_format = "data:image/jpeg;base64,";

                    // 计算需要的Base64缓冲区大小 (输入长度 * 4/3 + 填充)
                    size_t olen = ((jpeg.len + 2) / 3) * 4 + strlen(image_format) + 1;  // +1 for null terminator

                    // 分配内存并检查是否成功
                    unsigned char *base64_image = (uint8_t*)heap_caps_aligned_alloc(16, olen, MALLOC_CAP_SPIRAM);
                    if (!base64_image) {
                        ESP_LOGE(TAG, "Failed to allocate memory for base64 encoding: %u bytes", olen);
                        if (jpeg.data) {
                            heap_caps_free(jpeg.data);
                        }
                        return;
                    }

                    // 清零缓冲区
                    memset(base64_image, 0, olen);

                    // 添加Base64前缀
                    strcpy((char *)base64_image, image_format);

                    // 进行Base64编码
                    size_t encoded_size = 0;
                    int ret = mbedtls_base64_encode(base64_image + strlen(image_format), olen - strlen(image_format), &encoded_size, jpeg.data, jpeg.len);
                    if (ret == 0) {
                        protocol_->SendLlmImage((const char *)base64_image, strlen(image_format) + encoded_size, 0, request, 0);
                        ESP_LOGI(TAG, "Successfully encoded JPEG to base64, size: %u", olen);
                    } else {
                        ESP_LOGE(TAG, "mbedtls_base64_encode failed: %d", ret);
                    }

                    // 释放内存
                    heap_caps_free(base64_image);
                    if (jpeg.data) {
                        heap_caps_free(jpeg.data);
                    }
                } else {
                    ESP_LOGE(TAG, "Failed to get captured JPEG");
                }
            }
        });
    }
}

void Application::TouchActive(int value_head, int value_body) {
    // ESP_LOGI(TAG, "TouchActive value_head:%d, value_body:%d", value_head, value_body);
    if (device_state_ != kDeviceStateIdle) {
        return;
    }
    static int last_value_head = value_head;
    static int last_value_body = value_body;
    static int header_count = 0;
    if (touch_count_ == 0) {
        if (device_state_ == kDeviceStateListening) {
            if (device_state_ == kDeviceStateListening) {
                if (value_head > last_value_head * 1.1) {
                    touch_count_ = 50;
                    protocol_->SendMcpMessage("正在抚摸你的头，请提供相关的情绪价值，回答");
                } else if (value_body > last_value_body * 1.1) {
                    touch_count_ = 50;
                    protocol_->SendMcpMessage("正在抚摸你的身体，请提供相关的情绪价值，回答");
                }
            }
        } else if (device_state_ == kDeviceStateIdle) {
            if (touch_active_) {
                // ESP_LOGI(TAG, "TouchActive value_head:%.2f, value_body:%.2f", (float)value_head / (float)last_value_head, (float)value_body / (float)last_value_body);
                if (value_head > last_value_head * 1.05) {
                    touch_count_ = 15;
                    Board::GetInstance().SetPowerSaveMode(false);
                    Board::GetInstance().GetDisplay()->SetEmotion("happy");
                    Board::GetInstance().MotorStartKick(500);
                    Schedule([this]() {
                        PlaySound(Lang::Sounds::OGG_YING);
                    });
                    TouchRestoreTimer(3000);
                } else if (value_body > last_value_body * 1.05) {
                    touch_count_ = 15;
                    Board::GetInstance().SetPowerSaveMode(false);
                    Board::GetInstance().GetDisplay()->SetEmotion("loving");
                    Board::GetInstance().MotorStartKick(500);
                    Schedule([this]() {
                        PlaySound(Lang::Sounds::OGG_SATISFY);
                    });
                    TouchRestoreTimer(3000);
                }
            } else if (value_head > last_value_head * 1.05) {
                touch_count_ = 30;
                header_count = 1;
                ESP_LOGW(TAG, "TouchActive tick header start!!!!");
            }
        }
    } else {
        if (device_state_ == kDeviceStateIdle && !touch_active_) {
            if (value_head > last_value_head * 1.05 && header_count > 0) {
                header_count++;
                ESP_LOGW(TAG, "TouchActive tick header %d", header_count);
                if (header_count >= 3) {
                    ESP_LOGW(TAG, "TouchActive tick header checked!!!!");
                    header_count = 0;
                    touch_count_ = 20;

                    auto& board = Board::GetInstance();
                    board.SetPowerSaveMode(false);
                    board.MotorStartKick(1500);
                    board.GetDisplay()->SetEmotion("neutral");
                    touch_active_ = true;
                    Schedule([this]() {
                        PlaySound(Lang::Sounds::OGG_WU_CURIOUS);
                    });
                }
            }
        }
    }
    if (touch_count_ > 0) {
        touch_count_--;
        if (touch_count_ == 0) {
            header_count = 0;
        }
    }
    last_value_head = value_head;
    last_value_body = value_body;
}

void Application::Shake(float mag, float delta, bool is_strong) {
    ESP_LOGI(TAG,"Shake mag:%f, delta:%f, is_strong:%d", mag, delta, is_strong);
    if (device_state_ != kDeviceStateIdle) {
        return;
    }
    if (!protocol_) {
        return;
    }

    if (device_state_ == kDeviceStateIdle) {
        // if (is_strong) {
        //     ToggleChatState();
        //     Schedule([this]() {
        //         protocol_->SendMcpMessage("你被轻轻摇醒。请以激动的情绪表达。");
        //     });
        // }
        if (touch_active_) {
            bool confused = false;
            if (is_strong) {
            //     confused = true;
            // } else {
                static TickType_t pre_time = 0;
                static int shake_count = 0;
                TickType_t cur_time = xTaskGetTickCount();
                if (cur_time - pre_time < pdMS_TO_TICKS(1000)) {
                    shake_count++;
                } else {
                    shake_count = 0;
                }
                pre_time = cur_time;
                if (shake_count >= 2) {
                    confused = true;
                    shake_count = 0;
                }
            }
            if (confused) {
                Board::GetInstance().SetPowerSaveMode(false);
                Board::GetInstance().GetDisplay()->SetEmotion("confused");
                Board::GetInstance().MotorStartKick(1500);
                Schedule([this]() {
                    PlaySound(Lang::Sounds::OGG_WUWU);
                });
                TouchRestoreTimer(5000);
            }
        }
    }
    if (device_state_ == kDeviceStateListening) {
        if (is_strong) {
            protocol_->SendMcpMessage("你被剧烈摇晃。请以委屈/迷糊/激动等情绪表达。");
        } else if (device_state_ == kDeviceStateListening) {
            protocol_->SendMcpMessage("正在轻微摇晃你。请以平和、安抚的语气回应，如果连续2次以上以委屈/迷糊/激动等情绪表达。");
        }
    }
}

void Application::LiftUp() {
    ESP_LOGI(TAG, " LiftUp");
    if (device_state_ == kDeviceStateListening) {
        protocol_->SendMcpMessage("你被举高高。请以快乐、兴奋的语气回应。");
    }
}

void Application::TouchRestoreTimer(int duration) {
    if (touch_timer_) {
        xTimerStop(touch_timer_, 0);
        xTimerChangePeriod(touch_timer_, pdMS_TO_TICKS(duration), 0);
        xTimerStart(touch_timer_, 0);
    }
}
void Application::TouchRestoreTimerCb(TimerHandle_t xTimer) {
    auto* self = static_cast<Application*>(pvTimerGetTimerID(xTimer));
    if (self) {
        self->TouchRestore();
    }
}
void Application::TouchRestore() {
    if (device_state_ == kDeviceStateIdle) {
        Board::GetInstance().GetDisplay()->SetEmotion("neutral");
    }
}

void Application::StartRing() {
    ESP_LOGI(TAG,"StartRing");
    if (ring_timer_) {
        ringing_ = true;
        xTimerStop(ring_timer_, 0);
        xTimerChangePeriod(ring_timer_, pdMS_TO_TICKS(2000), 0);
        xTimerStart(ring_timer_, 0);
    }
}
void Application::RingTimerCb(TimerHandle_t xTimer) {
    auto* self = static_cast<Application*>(pvTimerGetTimerID(xTimer));
    if (self && self->ringing_) {
        self->Schedule([self]() {
            self->PlaySound(Lang::Sounds::OGG_RING);
        });
    }
}
void Application::StopRing() {
    ESP_LOGI(TAG,"StopRing");
    ringing_ = false;
    xTimerStop(ring_timer_, 0);
    GetAudioService().ResetDecoder();
}

AlarmError Application::SetAlarmTime(const std::string& type, const std::string& name, int target_time_s, bool override) {
    ESP_LOGI(TAG, "SetAlarmTime: %s", name.c_str());
    if (target_time_s <= 0) {
        ESP_LOGW(TAG, "SetAlarmTime failed for target_time_s <= 0");
        return ALARM_ERROR_INVALID_ALARM_TIME;
    }
    if (!alarm_manager_) {
        ESP_LOGW(TAG, "SetAlarmTime failed for alarm_manager_ is null");
        return ALARM_ERROR_INVALID_ALARM_MANAGER;
    }
    if (alarm_manager_->HasActiveAlarm() && !override) {
        ESP_LOGW(TAG, "SetAlarmTime for alarm already exists");
        return ALARM_ERROR_TOO_MANY_ALARMS;
    }

    Schedule([this, type, name, target_time_s, override]() {
        if (!alarm_manager_) {
            return;
        }
        auto error = alarm_manager_->SetAlarm(type, name, target_time_s, override);
        if (error != ALARM_ERROR_NONE) {
            ESP_LOGW(TAG, "SetAlarmTime failed for error:%d", (int)error);
        }
    });
    return ALARM_ERROR_NONE;
}

bool Application::GetAlarmList(std::vector<AlarmInfo>& out_list) {
    ESP_LOGI(TAG, "GetAlarmList");
    if (!alarm_manager_) {
        ESP_LOGE(TAG, "GetAlarmList failed: alarm_manager_ is null");
        out_list.clear();
        return false;
    }
    return alarm_manager_->GetAlarmList(out_list);
}

bool Application::CancelAlarm() {
    ESP_LOGI(TAG, "CancelAlarm");
    if (!alarm_play_timer_handle_ && (!alarm_manager_ || !alarm_manager_->HasActiveAlarm())) {
        return false;
    }
    Schedule([this]() {
        if (alarm_manager_) {
            alarm_manager_->ClearAll();
        }
        StopAlarmRinging();
    });
    return true;
}

bool Application::StopAlarmRinging() {
    if (!alarm_play_timer_handle_) {
        return false;
    }

    Schedule([this]() {
        if (!alarm_play_timer_handle_) {
            return;
        }
        esp_err_t err = esp_timer_stop(alarm_play_timer_handle_);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "CancelAlarm: esp_timer_stop failed, err:%d", err);
        }
        err = esp_timer_delete(alarm_play_timer_handle_);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "CancelAlarm: esp_timer_delete failed, err:%d", err);
        }
        alarm_play_timer_handle_ = nullptr;
        ringing_alarm_name_.clear();
        audio_service_.ResetDecoder();
        ESP_LOGI(TAG, "Stop alarm ringing success");
    });
    return true;
}

void Application::OnAlarmCallback(const std::string& name, const std::string& format_time) {
    ESP_LOGI(TAG, "OnAlarm, name:%s time:%s", name.c_str(), format_time.c_str());
    StopAlarmRinging();
    // 通过 Schedule 切换到主线程执行，快速返回释放 AlarmManager 的锁
    Schedule([this, name]() {
        ringing_alarm_name_ = name;
        // 创建周期定时器播放闹钟
        esp_timer_create_args_t clock_timer_args = {
            .callback = [](void* arg) {
                Application* app = static_cast<Application*>(arg);
                app->Schedule([app]() {
                    if (app->GetDeviceState() != kDeviceStateSpeaking) {
                        auto& audio_service = app->GetAudioService();
                        audio_service.ResetDecoder();
                        audio_service.PlaySound(Lang::Sounds::OGG_ALARM);
                    }

                    if (app->GetDeviceState() != kDeviceStateSpeaking) {
                        auto* display = Board::GetInstance().GetDisplay();
                        if (display) {
                            display->SetStatus("闹钟");
                            if (app->ringing_alarm_name_.empty()) {
                                display->SetChatMessage("system", "闹钟：闹钟提醒~~");
                            } else {
                                display->SetChatMessage("system", ("闹钟：" + app->ringing_alarm_name_ + "~~").c_str());
                            }
                        }
                    }
                });
            },
            .arg = this,
            .dispatch_method = ESP_TIMER_TASK,
            .name = "alarm_play_timer",
            .skip_unhandled_events = true
        };

        esp_timer_create(&clock_timer_args, &alarm_play_timer_handle_);
        esp_timer_start_periodic(alarm_play_timer_handle_, 3000000);  // 3秒周期

        ESP_LOGI(TAG, "Alarm timer started for: %s", ringing_alarm_name_.c_str());
    });
}

void Application::SetAISleep() {
    if (!protocol_) {
        ESP_LOGE(TAG, "SetAISleep: Protocol not initialized");
        return;
    }

    protocol_->SetAISleep();
}

void Application::ReadNertcConfig() {
#if NERTC_ENABLE_CONFIG_FILE
    if (!NeRtcProtocol::MountFileSystem()) {
        ESP_LOGE(TAG, "Failed to initialize file system");
        return;
    }

    auto* config_json = NeRtcProtocol::ReadConfigJson();
    if(config_json) {
        cJSON* custom_config = cJSON_GetObjectItem(config_json, "custom_config");
        if (custom_config && cJSON_IsObject(custom_config)) {
            cJSON* test_mode = cJSON_GetObjectItem(custom_config, "test_mode");
            if(test_mode && cJSON_IsBool(test_mode)) {
                enable_test_mode_ = (test_mode->valueint ? true : false);
                ESP_LOGI(TAG, "local config set test mode to %d", enable_test_mode_ ? 1 : 0);
            }
        }
        cJSON_Delete(config_json);
    } else{
        ESP_LOGW(TAG, "no local config file");
    }
#endif
}

//
void Application::TestDestroy() {
    if (protocol_) {
        protocol_->TestDestroy();
    }
}

void Application::Close() {
    if (device_state_ == kDeviceStateActivating) {
        SetDeviceState(kDeviceStateIdle);
        return;
    }
    SetDeviceState(kDeviceStateIdle);
    ESP_LOGW(TAG, "Stop alarm ringing for app close");
    StopAlarmRinging();

    if (!protocol_) {
        ESP_LOGE(TAG, "Protocol not initialized");
        return;
    }

    Schedule([this]() {
        protocol_->CloseAudioChannel();
    });
}

void Application::ParseSongListFromJson(const std::string& json, std::vector<MusicInfo>& out_list, bool& play_now){
    out_list.clear();
    cJSON* root = cJSON_Parse(json.c_str());
    if (!root) {
        ESP_LOGE(TAG, "ParseSongListFromJson: Failed to parse JSON");
        return;
    }

    cJSON* need_confirm = cJSON_GetObjectItem(root, "need_confirm");
    if(cJSON_IsBool(need_confirm) && need_confirm->valueint == 0){
        play_now = true;
    }
    auto song_list = cJSON_GetObjectItem(root, "song_list");
    if (!cJSON_IsArray(song_list) || cJSON_GetArraySize(song_list) == 0) {
        ESP_LOGE(TAG, "ParseSongListFromJson: 'song_list' is not an array");
        cJSON_Delete(root);
        return;
    }
    out_list.resize(cJSON_GetArraySize(song_list));
    for (int i = 0; i < cJSON_GetArraySize(song_list); ++i) {
        cJSON* song_item = cJSON_GetArrayItem(song_list, i);
        if (cJSON_IsObject(song_item)) {
            MusicInfo music_info;
            cJSON* name = cJSON_GetObjectItem(song_item, "name");
            cJSON* uri = cJSON_GetObjectItem(song_item, "url");
            cJSON* album = cJSON_GetObjectItem(song_item, "album");
            cJSON* artist = cJSON_GetObjectItem(song_item, "artist");
            cJSON* index = cJSON_GetObjectItem(song_item, "index");
            if(!cJSON_IsNumber(index) || index->valueint >= cJSON_GetArraySize(song_list)){
                ESP_LOGI(TAG, "ParseSongListFromJson: invalid song index");
                cJSON_Delete(root);
                return;
            }
            if (cJSON_IsString(name)) {
                music_info.name = name->valuestring;
            }
            if (cJSON_IsString(uri)) {
                music_info.uri = uri->valuestring;
            }
            if (cJSON_IsString(album)) {
                music_info.album = album->valuestring;
            }
            if (cJSON_IsString(artist)) {
                music_info.artist = artist->valuestring;
            }
            out_list[index->valueint] = music_info;
            ESP_LOGI(TAG, "ParseSongListFromJson: Parsed song - Name: %s, URI: %s, Album: %s, Artist: %s", 
                     music_info.name.c_str(), music_info.uri.c_str(), music_info.album.c_str(), music_info.artist.c_str());
        }
    }
    cJSON_Delete(root);
}
