/*
 * MCP Server Implementation
 * Reference: https://modelcontextprotocol.io/specification/2024-11-05
 */

#include "mcp_server.h"
#include <esp_log.h>
#include <esp_app_desc.h>
#include <algorithm>
#include <cstring>
#include <esp_pthread.h>

#include "application.h"
#include "display.h"
#include "oled_display.h"
#include "board.h"
#include "settings.h"
#include "lvgl_theme.h"
#include "lvgl_display.h"

#define TAG "MCP"

McpServer::McpServer() {
}

McpServer::~McpServer() {
    disabled_tools_.clear();
    for (auto tool : tools_) {
        delete tool;
    }
    tools_.clear();
}

void McpServer::AddCommonTools() {
    // *Important* To speed up the response time, we add the common tools to the beginning of
    // the tools list to utilize the prompt cache.
    // **重要** 为了提升响应速度，我们把常用的工具放在前面，利用 prompt cache 的特性。

    // Backup the original tools list and restore it after adding the common tools.
    auto original_tools = std::move(tools_);
    auto& board = Board::GetInstance();

    // Do not add custom tools here.
    // Custom tools must be added in the board's InitializeTools function.

    AddTool("self.get_device_status",
        "Provides the real-time information of the device, including the current status of the audio speaker, screen, battery, network, etc.\n"
        "Use this tool for: \n"
        "1. Answering questions about current condition (e.g. what is the current volume of the audio speaker?)\n"
        "2. As the first step to control the device (e.g. turn up / down the volume of the audio speaker, etc.)",
        PropertyList(),
        [&board](const PropertyList& properties) -> ReturnValue {
            return board.GetDeviceStatusJson();
        });

    AddTool("self.good_bye",
        "用户有明确离开意图的时候，比如说“再见”、“我要休息啦”、“拜拜啦”、“goodbye”、“byebye”等等，调用它。",
        PropertyList(),
        [](const PropertyList& properties) -> ReturnValue {
            auto& app = Application::GetInstance();
            app.SetAISleep();
            return true;
        });

    AddTool("self.audio_speaker.set_volume",
        "Set the volume of the audio speaker. If the current volume is unknown, you must call `self.get_device_status` tool first and then call this tool.",
        PropertyList({
            Property("volume", kPropertyTypeInteger, 0, 100)
        }),
        [&board](const PropertyList& properties) -> ReturnValue {
            auto codec = board.GetAudioCodec();
            codec->SetOutputVolume(properties["volume"].value<int>());
            return true;
        });

    auto backlight = board.GetBacklight();
    if (backlight) {
        AddTool("self.screen.set_brightness",
            "Set the brightness of the screen.",
            PropertyList({
                Property("brightness", kPropertyTypeInteger, 0, 100)
            }),
            [backlight](const PropertyList& properties) -> ReturnValue {
                uint8_t brightness = static_cast<uint8_t>(properties["brightness"].value<int>());
                backlight->SetBrightness(brightness, true);
                return true;
            });
    }

#ifdef HAVE_LVGL
    auto display = board.GetDisplay();
    if (display && display->GetTheme() != nullptr) {
        AddTool("self.screen.set_theme",
            "Set the theme of the screen. The theme can be `light` or `dark`.",
            PropertyList({
                Property("theme", kPropertyTypeString)
            }),
            [display](const PropertyList& properties) -> ReturnValue {
                auto theme_name = properties["theme"].value<std::string>();
                auto& theme_manager = LvglThemeManager::GetInstance();
                auto theme = theme_manager.GetTheme(theme_name);
                if (theme != nullptr) {
                    display->SetTheme(theme);
                    return true;
                }
                return false;
            });
    }

    auto camera = board.GetCamera();
    if (camera) {
#ifdef CONFIG_CONNECTION_TYPE_NERTC
        AddTool("self.photo_explain",
            "全能视觉与拍照工具。这是你的‘眼睛’。当用户涉及到任何视觉相关的请求时，必须调用此工具。\n"
            "功能范围：\n"
            "1. 拍照/看世界：响应如“拍一张照片”、“看看这是什么”、“我拍到了什么”、“帮我拍个照”等指令。\n"
            "2. 识别与分析：响应如“这是什么东西”、“识别一下”、“看看这个场景”、“描述画面”、“用一首诗描述当前的场景”等指令。\n"
            "3. 功能性视觉：响应如“翻译一下这个”、“这道题怎么解”、“读一下上面的文字”等指令。注意：调用此工具意味着你会获取当前的视觉画面（自动拍照或读取画面）并根据question参数进行分析。不需要区分是单纯拍照还是解释，统一使用此工具。\n"
            "注意：调用此工具意味着你会获取当前的视觉画面（自动拍照或读取画面）并根据question参数进行分析。"
            "不需要区分是单纯拍照还是解释，统一使用此工具。\n"
            "参数：pre_answer，生成3-5字的简短口语回应，必须体现‘正在观看’或‘准备观察’的视觉动作，例如‘让我看看’、‘我瞧瞧看’、‘正在看喔’、‘Let me see’。严格禁止使用‘好的’、‘收到’、‘没问题’等无视觉语义的通用确认词。\n"
            "参数：question，用户的原始问题，不要做任何总结和修改。\n",
            PropertyList({
                Property("pre_answer", kPropertyTypeString),
                Property("question", kPropertyTypeString)
            }),
            [this](const PropertyList& properties) -> ReturnValue {
                try {
                    auto question = properties["question"].value<std::string>();
                    auto pre_answer = properties["pre_answer"].value<std::string>();

                    auto& app = Application::GetInstance();
                    if (app.GetDeviceState() == kDeviceStateIdle) {
                        ESP_LOGE(TAG, "Unsupport explain for ai stop");
                        return "{\"success\":false,\"error\":" "\"当前状态不支持识别操作\"}";
                    }

                    std::string query = "围绕这个主题《" + question + "》，分析并描述你看到了什么。";
                    app.PhotoExplain(query, pre_answer, false);
                    return "{\"success\":true,\"message\":\"识别成功\"}";
                } catch (const std::exception &e) {
                    ESP_LOGE(TAG, "Error interpreting recent photo: %s", e.what());
                    return "{\"success\":false,\"error\":\"" + std::string(e.what()) + "\"}";
                }
            });
#else
        AddTool("self.camera.take_photo",
            "Take a photo and explain it. Use this tool after the user asks you to see something.\n"
            "Args:\n"
            "  `question`: The question that you want to ask about the photo.\n"
            "Return:\n"
            "  A JSON object that provides the photo information.",
            PropertyList({
                Property("question", kPropertyTypeString)
            }),
            [camera](const PropertyList& properties) -> ReturnValue {
                // Lower the priority to do the camera capture
                TaskPriorityReset priority_reset(1);

                if (!camera->Capture()) {
                    throw std::runtime_error("Failed to capture photo");
                }
                auto question = properties["question"].value<std::string>();
                return camera->Explain(question);
            });
#endif
    }
#endif

    AddTool("self.get_alarm_list",
        "获取当前已经设置的闹钟列表，返回结果里会有一个list的JSON数组，数组每个元素是一个闹钟对象。如果没有设置闹钟，则数组为空。",
        PropertyList(),
        [](const PropertyList& properties) -> ReturnValue {
            auto& instance = Application::GetInstance();
            std::vector<AlarmInfo> alarm_list;
            bool success = instance.GetAlarmList(alarm_list);
            if (!success) {
                return  "{\"success\": false, \"message\": \"get alarm list failed\"}";
            } else {
                cJSON* root = cJSON_CreateObject();
                cJSON_AddBoolToObject(root, "success", true);
                cJSON_AddStringToObject(root, "message", "get alarm list success");

                cJSON* list = cJSON_CreateArray();
                for (const auto& alarm : alarm_list) {
                    cJSON* item = cJSON_CreateObject();
                    cJSON_AddStringToObject(item, "name", alarm.name.c_str());
                    cJSON_AddStringToObject(item, "time", alarm.format_time.c_str());
                    cJSON_AddItemToArray(list, item);
                }
                cJSON_AddItemToObject(root, "list", list);

                char* json_str = cJSON_PrintUnformatted(root);
                std::string result(json_str);
                cJSON_free(json_str);
                cJSON_Delete(root);

                return result;
            }
        });

    AddTool("self.set_alarm",
        "设置闹钟和提醒的工具，用户可能会说“明天早上8点叫我”或“10分钟后提醒我”，你必须根据当前时间计算出目标时间。在调用这个工具之前必须先调用get_alarm_list工具确定当前是否已经有闹钟存在了\n"
        "参数：alarm_type，表示闹钟类型，值为以下2个值里二选一：\"one_off\", \"countdown\"，表示：一次性、倒计时\n"
        "参数：time_expression，表示闹钟时间，如果钟类型是countdown，填'1200'(秒)；如果钟类型是 one_off，必须填 'YYYY-MM-DD HH:MM:SS' 格式的绝对时间\n"
        "参数：name，表示闹钟名称（可选，默认值为空）\n"
        "参数：override，表示是否要覆盖之前设置的闹钟，如果前一个get_alarm_list工具获取到的闹钟列表为空，该值设置为0；其他情况：如获取失败或闹钟列表不为空等等，都设置为1\n"
        "参数：current_time，获取当前北京时间（格式：YYYY-MM-DD HH:MM:SS）\n",
        PropertyList({
            Property("alarm_type", kPropertyTypeString),
            Property("time_expression", kPropertyTypeString),
            Property("name", kPropertyTypeString),
            Property("override", kPropertyTypeInteger),
            Property("current_time", kPropertyTypeString),
        }),
        [](const PropertyList& properties) -> ReturnValue {
            ESP_LOGI(TAG, "set_alarm: %s", properties.to_json().c_str());

            std::string alarm_type = properties["alarm_type"].value<std::string>();
            std::string time_expression = properties["time_expression"].value<std::string>();
            std::string name = properties["name"].value<std::string>();
            bool override = properties["override"].value<int>() > 0 ? true : false;
            std::string current_time = properties["current_time"].value<std::string>();

            int target_time_s = 0;
            if (alarm_type == "countdown") {
                // 倒计时模式：time_expression 是秒数
                char* endptr = nullptr;
                long seconds = strtol(time_expression.c_str(), &endptr, 10);

                if (endptr == time_expression.c_str() || *endptr != '\0' || seconds <= 0) {
                    ESP_LOGE(TAG, "Invalid countdown seconds: %s", time_expression.c_str());
                    return "{\"success\": false, \"message\": \"Set failed because invalid countdown seconds\"}";
                }

                target_time_s = static_cast<int>(seconds);
                ESP_LOGI(TAG, "Countdown alarm set for %d seconds", target_time_s);
            } else if (alarm_type == "one_off") {
                // 一次性模式：time_expression 是绝对时间 YYYY-MM-DD HH:MM:SS
                auto parse_datetime = [](const std::string& time_str) -> time_t {
                    struct tm tm = {};
                    int year, month, day, hour, minute, second;
                    if (sscanf(time_str.c_str(), "%d-%d-%d %d:%d:%d",
                            &year, &month, &day, &hour, &minute, &second) == 6) {
                        tm.tm_year = year - 1900;
                        tm.tm_mon = month - 1;
                        tm.tm_mday = day;
                        tm.tm_hour = hour;
                        tm.tm_min = minute;
                        tm.tm_sec = second;
                        tm.tm_isdst = -1;
                        return mktime(&tm);
                    }
                    return -1;
                };

                time_t target_timestamp = parse_datetime(time_expression);
                if (target_timestamp == -1) {
                    ESP_LOGE(TAG, "Failed to parse time: %s", time_expression.c_str());
                    return "{\"success\": false, \"message\": \"Set failed because invalid time format, expected YYYY-MM-DD HH:MM:SS\"}";
                }

                 // 优先使用 current_time 参数，不合法则使用本地时间
                time_t current_timestamp = parse_datetime(current_time);
                if (current_timestamp == -1) {
                    ESP_LOGW(TAG, "Invalid current_time: '%s', using local time", current_time.c_str());
                    current_timestamp = time(NULL);
                }

                target_time_s = static_cast<int>(target_timestamp - current_timestamp);
                if (target_time_s <= 0) {
                    ESP_LOGE(TAG, "Target time is in the past. Target: %s, Current: %s",
                            time_expression.c_str(), current_time.c_str());
                    return "{\"success\": false, \"message\": \"Set failed because target time is in the past\"}";
                }

                ESP_LOGI(TAG, "One-off alarm set for %d seconds from now (target: %s)",
                        target_time_s, time_expression.c_str());
            } else {
                ESP_LOGE(TAG, "Invalid alarm_type: %s", alarm_type.c_str());
                return "{\"success\": false, \"message\": \"Set failed because invalid alarm_type, expected 'one_off' or 'countdown'\"}";
            }

            auto& instance = Application::GetInstance();
            AlarmError error = instance.SetAlarmTime(alarm_type, name, target_time_s, override);
            switch (error) {
                case ALARM_ERROR_NONE:
                    return "{\"success\": true, \"message\": \"Alarm set successfully\"}";
                case ALARM_ERROR_TOO_MANY_ALARMS:
                    return "{\"success\": false, \"message\": \"Set failed because an alarm already exists\"}";
                case ALARM_ERROR_INVALID_ALARM_TIME:
                    return "{\"success\": false, \"message\": \"Set failed because invalid alarm time\"}";
                case ALARM_ERROR_INVALID_ALARM_MANAGER:
                    return "{\"success\": false, \"message\": \"Set failed because invalid alarm state\"}";
                default:
                    return "{\"success\": false, \"message\": \"Set failed because unknown error\"}";
            }
        });

    AddTool("self.cancel_alarm",
        "识别用户取消闹钟的意图，无论当前是否有闹钟都要触发该工具。",
        PropertyList(),
        [](const PropertyList& properties) -> ReturnValue {
            auto& instance = Application::GetInstance();
            bool success = instance.CancelAlarm();
            return success ? "{\"success\": true, \"message\": \"Cancel alarm successfully\"}" : "{\"success\": false, \"message\": \"Cancel alarm failed because no alarm exist\"}";
        });

    // Restore the original tools list to the end of the tools list
    tools_.insert(tools_.end(), original_tools.begin(), original_tools.end());
}

void McpServer::AddUserOnlyTools() {
    // System tools
    AddUserOnlyTool("self.get_system_info",
        "Get the system information",
        PropertyList(),
        [this](const PropertyList& properties) -> ReturnValue {
            auto& board = Board::GetInstance();
            return board.GetSystemInfoJson();
        });

    AddUserOnlyTool("self.reboot", "Reboot the system",
        PropertyList(),
        [this](const PropertyList& properties) -> ReturnValue {
            auto& app = Application::GetInstance();
            app.Schedule([&app]() {
                ESP_LOGW(TAG, "User requested reboot");
                vTaskDelay(pdMS_TO_TICKS(1000));

                app.Reboot();
            });
            return true;
        });

    // Firmware upgrade
    AddUserOnlyTool("self.upgrade_firmware", "Upgrade firmware from a specific URL. This will download and install the firmware, then reboot the device.",
        PropertyList({
            Property("url", kPropertyTypeString, "The URL of the firmware binary file to download and install")
        }),
        [this](const PropertyList& properties) -> ReturnValue {
            auto url = properties["url"].value<std::string>();
            ESP_LOGI(TAG, "User requested firmware upgrade from URL: %s", url.c_str());

            auto& app = Application::GetInstance();
            app.Schedule([url, &app]() {
                auto ota = std::make_unique<Ota>();

                bool success = app.UpgradeFirmware(*ota, url);
                if (!success) {
                    ESP_LOGE(TAG, "Firmware upgrade failed");
                }
            });

            return true;
        });

    // Display control
#ifdef HAVE_LVGL
    auto display = dynamic_cast<LvglDisplay*>(Board::GetInstance().GetDisplay());
    if (display) {
        AddUserOnlyTool("self.screen.get_info", "Information about the screen, including width, height, etc.",
            PropertyList(),
            [display](const PropertyList& properties) -> ReturnValue {
                cJSON *json = cJSON_CreateObject();
                cJSON_AddNumberToObject(json, "width", display->width());
                cJSON_AddNumberToObject(json, "height", display->height());
                if (dynamic_cast<OledDisplay*>(display)) {
                    cJSON_AddBoolToObject(json, "monochrome", true);
                } else {
                    cJSON_AddBoolToObject(json, "monochrome", false);
                }
                return json;
            });

#if CONFIG_LV_USE_SNAPSHOT
        AddUserOnlyTool("self.screen.snapshot", "Snapshot the screen and upload it to a specific URL",
            PropertyList({
                Property("url", kPropertyTypeString),
                Property("quality", kPropertyTypeInteger, 80, 1, 100)
            }),
            [display](const PropertyList& properties) -> ReturnValue {
                auto url = properties["url"].value<std::string>();
                auto quality = properties["quality"].value<int>();

                std::string jpeg_data;
                if (!display->SnapshotToJpeg(jpeg_data, quality)) {
                    throw std::runtime_error("Failed to snapshot screen");
                }

                ESP_LOGI(TAG, "Upload snapshot %u bytes to %s", jpeg_data.size(), url.c_str());

                // 构造multipart/form-data请求体
                std::string boundary = "----ESP32_SCREEN_SNAPSHOT_BOUNDARY";

                auto http = Board::GetInstance().GetNetwork()->CreateHttp(3);
                http->SetHeader("Content-Type", "multipart/form-data; boundary=" + boundary);
                if (!http->Open("POST", url)) {
                    throw std::runtime_error("Failed to open URL: " + url);
                }
                {
                    // 文件字段头部
                    std::string file_header;
                    file_header += "--" + boundary + "\r\n";
                    file_header += "Content-Disposition: form-data; name=\"file\"; filename=\"screenshot.jpg\"\r\n";
                    file_header += "Content-Type: image/jpeg\r\n";
                    file_header += "\r\n";
                    http->Write(file_header.c_str(), file_header.size());
                }

                // JPEG数据
                http->Write((const char*)jpeg_data.data(), jpeg_data.size());

                {
                    // multipart尾部
                    std::string multipart_footer;
                    multipart_footer += "\r\n--" + boundary + "--\r\n";
                    http->Write(multipart_footer.c_str(), multipart_footer.size());
                }
                http->Write("", 0);

                if (http->GetStatusCode() != 200) {
                    throw std::runtime_error("Unexpected status code: " + std::to_string(http->GetStatusCode()));
                }
                std::string result = http->ReadAll();
                http->Close();
                ESP_LOGI(TAG, "Snapshot screen result: %s", result.c_str());
                return true;
            });

        AddUserOnlyTool("self.screen.preview_image", "Preview an image on the screen",
            PropertyList({
                Property("url", kPropertyTypeString)
            }),
            [display](const PropertyList& properties) -> ReturnValue {
                auto url = properties["url"].value<std::string>();
                auto http = Board::GetInstance().GetNetwork()->CreateHttp(3);

                if (!http->Open("GET", url)) {
                    throw std::runtime_error("Failed to open URL: " + url);
                }
                int status_code = http->GetStatusCode();
                if (status_code != 200) {
                    throw std::runtime_error("Unexpected status code: " + std::to_string(status_code));
                }

                size_t content_length = http->GetBodyLength();
                char* data = (char*)heap_caps_malloc(content_length, MALLOC_CAP_8BIT);
                if (data == nullptr) {
                    throw std::runtime_error("Failed to allocate memory for image: " + url);
                }
                size_t total_read = 0;
                while (total_read < content_length) {
                    int ret = http->Read(data + total_read, content_length - total_read);
                    if (ret < 0) {
                        heap_caps_free(data);
                        throw std::runtime_error("Failed to download image: " + url);
                    }
                    if (ret == 0) {
                        break;
                    }
                    total_read += ret;
                }
                http->Close();

                auto image = std::make_unique<LvglAllocatedImage>(data, content_length);
                display->SetPreviewImage(std::move(image));
                return true;
            });
#endif // CONFIG_LV_USE_SNAPSHOT
    }
#endif // HAVE_LVGL

    // Assets download url
    auto& assets = Assets::GetInstance();
    if (assets.partition_valid()) {
        AddUserOnlyTool("self.assets.set_download_url", "Set the download url for the assets",
            PropertyList({
                Property("url", kPropertyTypeString)
            }),
            [](const PropertyList& properties) -> ReturnValue {
                auto url = properties["url"].value<std::string>();
                Settings settings("assets", true);
                settings.SetString("download_url", url);
                return true;
            });
    }
}

void McpServer::AddTool(McpTool* tool) {
    // Prevent adding duplicate tools
    if (std::find_if(tools_.begin(), tools_.end(), [tool](const McpTool* t) { return t->name() == tool->name(); }) != tools_.end()) {
        ESP_LOGW(TAG, "Tool %s already added", tool->name().c_str());
        return;
    }

    ESP_LOGI(TAG, "Add tool: %s%s", tool->name().c_str(), tool->user_only() ? " [user]" : "");
    tools_.push_back(tool);
}

void McpServer::AddTool(const std::string& name, const std::string& description, const PropertyList& properties, std::function<ReturnValue(const PropertyList&)> callback) {
    AddTool(new McpTool(name, description, properties, callback));
}

void McpServer::AddUserOnlyTool(const std::string& name, const std::string& description, const PropertyList& properties, std::function<ReturnValue(const PropertyList&)> callback) {
    auto tool = new McpTool(name, description, properties, callback);
    tool->set_user_only(true);
    AddTool(tool);
}

void McpServer::ParseMessage(const std::string& message) {
    cJSON* json = cJSON_Parse(message.c_str());
    if (json == nullptr) {
        ESP_LOGE(TAG, "Failed to parse MCP message: %s", message.c_str());
        return;
    }
    ParseMessage(json);
    cJSON_Delete(json);
}

void McpServer::ParseCapabilities(const cJSON* capabilities) {
    auto vision = cJSON_GetObjectItem(capabilities, "vision");
    if (cJSON_IsObject(vision)) {
        auto url = cJSON_GetObjectItem(vision, "url");
        auto token = cJSON_GetObjectItem(vision, "token");
        if (cJSON_IsString(url)) {
            auto camera = Board::GetInstance().GetCamera();
            if (camera) {
                std::string url_str = std::string(url->valuestring);
                std::string token_str;
                if (cJSON_IsString(token)) {
                    token_str = std::string(token->valuestring);
                }
                camera->SetExplainUrl(url_str, token_str);
            }
        }
    }

    auto disable_tools = cJSON_GetObjectItem(capabilities, "disableTools");
    if (cJSON_IsArray(disable_tools)) {
        disabled_tools_.clear(); // 清空之前的设置

        int array_size = cJSON_GetArraySize(disable_tools);
        for (int i = 0; i < array_size; ++i) {
            auto tool_name_item = cJSON_GetArrayItem(disable_tools, i);
            if (cJSON_IsString(tool_name_item)) {
                std::string tool_name = tool_name_item->valuestring;
                disabled_tools_.insert(tool_name);
                ESP_LOGI(TAG, "Disabled tool: %s", tool_name.c_str());
            }
        }
    }

}

void McpServer::ParseMessage(const cJSON* json) {
    // Check JSONRPC version
    auto version = cJSON_GetObjectItem(json, "jsonrpc");
    if (version == nullptr || !cJSON_IsString(version) || strcmp(version->valuestring, "2.0") != 0) {
        ESP_LOGE(TAG, "Invalid JSONRPC version: %s", version ? version->valuestring : "null");
        return;
    }

    // Check method
    auto method = cJSON_GetObjectItem(json, "method");
    if (method == nullptr || !cJSON_IsString(method)) {
        ESP_LOGE(TAG, "Missing method");
        return;
    }

    auto method_str = std::string(method->valuestring);
    if (method_str.find("notifications") == 0) {
        return;
    }

    // Check params
    auto params = cJSON_GetObjectItem(json, "params");
    if (params != nullptr && !cJSON_IsObject(params)) {
        ESP_LOGE(TAG, "Invalid params for method: %s", method_str.c_str());
        return;
    }

    auto id = cJSON_GetObjectItem(json, "id");
    if (id == nullptr || !cJSON_IsNumber(id)) {
        ESP_LOGE(TAG, "Invalid id for method: %s", method_str.c_str());
        return;
    }
    auto id_int = id->valueint;

    if (method_str == "initialize") {
        if (cJSON_IsObject(params)) {
            auto capabilities = cJSON_GetObjectItem(params, "capabilities");
            if (cJSON_IsObject(capabilities)) {
                ParseCapabilities(capabilities);
            }
        }
        auto app_desc = esp_app_get_description();
        std::string message = "{\"protocolVersion\":\"2024-11-05\",\"capabilities\":{\"tools\":{}},\"serverInfo\":{\"name\":\"" BOARD_NAME "\",\"version\":\"";
        message += app_desc->version;
        message += "\"}}";
        ReplyResult(id_int, message);
    } else if (method_str == "tools/list") {
        std::string cursor_str = "";
        bool list_user_only_tools = false;
        if (params != nullptr) {
            auto cursor = cJSON_GetObjectItem(params, "cursor");
            if (cJSON_IsString(cursor)) {
                cursor_str = std::string(cursor->valuestring);
            }
            auto with_user_tools = cJSON_GetObjectItem(params, "withUserTools");
            if (cJSON_IsBool(with_user_tools)) {
                list_user_only_tools = with_user_tools->valueint == 1;
            }
        }
        GetToolsList(id_int, cursor_str, list_user_only_tools);
    } else if (method_str == "tools/call") {
        if (!cJSON_IsObject(params)) {
            ESP_LOGE(TAG, "tools/call: Missing params");
            ReplyError(id_int, "Missing params");
            return;
        }
        auto tool_name = cJSON_GetObjectItem(params, "name");
        if (!cJSON_IsString(tool_name)) {
            ESP_LOGE(TAG, "tools/call: Missing name");
            ReplyError(id_int, "Missing name");
            return;
        }
        auto tool_arguments = cJSON_GetObjectItem(params, "arguments");
        if (tool_arguments != nullptr && !cJSON_IsObject(tool_arguments)) {
            ESP_LOGE(TAG, "tools/call: Invalid arguments");
            ReplyError(id_int, "Invalid arguments");
            return;
        }
        DoToolCall(id_int, std::string(tool_name->valuestring), tool_arguments);
    } else {
        ESP_LOGE(TAG, "Method not implemented: %s", method_str.c_str());
        ReplyError(id_int, "Method not implemented: " + method_str);
    }
}

void McpServer::ReplyResult(int id, const std::string& result) {
    std::string payload = "{\"jsonrpc\":\"2.0\",\"id\":";
    payload += std::to_string(id) + ",\"result\":";
    payload += result;
    payload += "}";
    Application::GetInstance().SendMcpMessage(payload);
}

void McpServer::ReplyError(int id, const std::string& message) {
    std::string payload = "{\"jsonrpc\":\"2.0\",\"id\":";
    payload += std::to_string(id);
    payload += ",\"error\":{\"message\":\"";
    payload += message;
    payload += "\"}}";
    Application::GetInstance().SendMcpMessage(payload);
}

void McpServer::GetToolsList(int id, const std::string& cursor, bool list_user_only_tools) {
    const int max_payload_size = 2000;
    std::string json = "{\"tools\":[";

    bool found_cursor = cursor.empty();
    auto it = tools_.begin();
    std::string next_cursor = "";

    while (it != tools_.end()) {
        if (IsToolDisabled((*it)->name())) {
            ++it;
            continue;
        }

        // 如果我们还没有找到起始位置，继续搜索
        if (!found_cursor) {
            if ((*it)->name() == cursor) {
                found_cursor = true;
            } else {
                ++it;
                continue;
            }
        }

        if (!list_user_only_tools && (*it)->user_only()) {
            ++it;
            continue;
        }

        // 添加tool前检查大小
        std::string tool_json = (*it)->to_json() + ",";
        if (json.length() + tool_json.length() + 30 > max_payload_size) {
            // 如果添加这个tool会超出大小限制，设置next_cursor并退出循环
            next_cursor = (*it)->name();
            break;
        }

        json += tool_json;
        ++it;
    }

    if (json.back() == ',') {
        json.pop_back();
    }

    if (json.back() == '[' && !tools_.empty()) {
        // 如果没有添加任何tool，返回错误
        ESP_LOGE(TAG, "tools/list: Failed to add tool %s because of payload size limit", next_cursor.c_str());
        ReplyError(id, "Failed to add tool " + next_cursor + " because of payload size limit");
        return;
    }

    if (next_cursor.empty()) {
        json += "]}";
    } else {
        json += "],\"nextCursor\":\"" + next_cursor + "\"}";
    }

    ReplyResult(id, json);
}

void McpServer::DoToolCall(int id, const std::string& tool_name, const cJSON* tool_arguments) {
    if (IsToolDisabled(tool_name)) {
        ESP_LOGE(TAG, "tools/call: Tool is disabled: %s", tool_name.c_str());
        ReplyError(id, "Tool is disabled: " + tool_name);
        return;
    }

    auto tool_iter = std::find_if(tools_.begin(), tools_.end(),
                                 [&tool_name](const McpTool* tool) {
                                     return tool->name() == tool_name;
                                 });

    if (tool_iter == tools_.end()) {
        ESP_LOGE(TAG, "tools/call: Unknown tool: %s", tool_name.c_str());
        ReplyError(id, "Unknown tool: " + tool_name);
        return;
    }

    PropertyList arguments = (*tool_iter)->properties();
    try {
        for (auto& argument : arguments) {
            bool found = false;
            if (cJSON_IsObject(tool_arguments)) {
                auto value = cJSON_GetObjectItem(tool_arguments, argument.name().c_str());
                if (argument.type() == kPropertyTypeBoolean && cJSON_IsBool(value)) {
                    argument.set_value<bool>(value->valueint == 1);
                    found = true;
                } else if (argument.type() == kPropertyTypeInteger && cJSON_IsNumber(value)) {
                    argument.set_value<int>(value->valueint);
                    found = true;
                } else if (argument.type() == kPropertyTypeString && cJSON_IsString(value)) {
                    argument.set_value<std::string>(value->valuestring);
                    found = true;
                }
            }

            if (!argument.has_default_value() && !found) {
                ESP_LOGE(TAG, "tools/call: Missing valid argument: %s", argument.name().c_str());
                ReplyError(id, "Missing valid argument: " + argument.name());
                return;
            }
        }
    } catch (const std::exception& e) {
        ESP_LOGE(TAG, "tools/call: %s", e.what());
        ReplyError(id, e.what());
        return;
    }

    // Use main thread to call the tool
    auto& app = Application::GetInstance();
    app.Schedule([this, id, tool_iter, arguments = std::move(arguments)]() {
        try {
            ReplyResult(id, (*tool_iter)->Call(arguments));
        } catch (const std::exception& e) {
            ESP_LOGE(TAG, "tools/call: %s", e.what());
            ReplyError(id, e.what());
        }
    });
}

bool McpServer::IsToolDisabled(const std::string& tool_name) const {
    return disabled_tools_.find(tool_name) != disabled_tools_.end();
}
