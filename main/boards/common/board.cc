#include "board.h"
#include "system_info.h"
#include "settings.h"
#include "display/display.h"
#include "display/oled_display.h"
#include "assets/lang_config.h"
#include "application.h"

#include <esp_log.h>
#include <esp_ota_ops.h>
#include <esp_chip_info.h>
#include <esp_random.h>

#define TAG "Board"

Board::Board() {
    Settings settings("board", true);
    uuid_ = settings.GetString("uuid");
    if (uuid_.empty()) {
        uuid_ = GenerateUuid();
        settings.SetString("uuid", uuid_);
    }
    ESP_LOGI(TAG, "UUID=%s SKU=%s", uuid_.c_str(), BOARD_NAME);
}

std::string Board::GenerateUuid() {
    // UUID v4 需要 16 字节的随机数据
    uint8_t uuid[16];

    // 使用 ESP32 的硬件随机数生成器
    esp_fill_random(uuid, sizeof(uuid));

    // 设置版本 (版本 4) 和变体位
    uuid[6] = (uuid[6] & 0x0F) | 0x40;    // 版本 4
    uuid[8] = (uuid[8] & 0x3F) | 0x80;    // 变体 1

    // 将字节转换为标准的 UUID 字符串格式
    char uuid_str[37];
    snprintf(uuid_str, sizeof(uuid_str),
        "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
        uuid[0], uuid[1], uuid[2], uuid[3],
        uuid[4], uuid[5], uuid[6], uuid[7],
        uuid[8], uuid[9], uuid[10], uuid[11],
        uuid[12], uuid[13], uuid[14], uuid[15]);

    return std::string(uuid_str);
}

bool Board::GetBatteryLevel(int &level, bool& charging, bool& discharging) {
    return false;
}

bool Board::GetTemperature(float& esp32temp){
    return false;
}

Display* Board::GetDisplay() {
    static NoDisplay display;
    return &display;
}

Camera* Board::GetCamera() {
    return nullptr;
}

Led* Board::GetLed() {
    static NoLed led;
    return &led;
}

std::string Board::GetSystemInfoJson() {
    /*
        {
            "version": 2,
            "flash_size": 4194304,
            "psram_size": 0,
            "minimum_free_heap_size": 123456,
            "mac_address": "00:00:00:00:00:00",
            "uuid": "00000000-0000-0000-0000-000000000000",
            "chip_model_name": "esp32s3",
            "chip_info": {
                "model": 1,
                "cores": 2,
                "revision": 0,
                "features": 0
            },
            "application": {
                "name": "my-app",
                "version": "1.0.0",
                "compile_time": "2021-01-01T00:00:00Z"
                "idf_version": "4.2-dev"
                "elf_sha256": ""
            },
            "partition_table": [
                "app": {
                    "label": "app",
                    "type": 1,
                    "subtype": 2,
                    "address": 0x10000,
                    "size": 0x100000
                }
            ],
            "ota": {
                "label": "ota_0"
            },
            "board": {
                ...
            }
        }
    */
    std::string json = R"({"version":2,"language":")" + std::string(Lang::CODE) + R"(",)";
    json += R"("flash_size":)" + std::to_string(SystemInfo::GetFlashSize()) + R"(,)";
    json += R"("minimum_free_heap_size":")" + std::to_string(SystemInfo::GetMinimumFreeHeapSize()) + R"(",)";
    json += R"("mac_address":")" + SystemInfo::GetMacAddress() + R"(",)";
    json += R"("uuid":")" + uuid_ + R"(",)";
    json += R"("chip_model_name":")" + SystemInfo::GetChipModelName() + R"(",)";

    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    json += R"("chip_info":{)";
    json += R"("model":)" + std::to_string(chip_info.model) + R"(,)";
    json += R"("cores":)" + std::to_string(chip_info.cores) + R"(,)";
    json += R"("revision":)" + std::to_string(chip_info.revision) + R"(,)";
    json += R"("features":)" + std::to_string(chip_info.features) + R"(},)";

    auto app_desc = esp_app_get_description();
    json += R"("application":{)";

    //capablities add
    json += R"("capabilities":{"netease_cloud_music":{"support_play":)";
    json += (GetBoardType() == std::string("wifi") ? "true" : "false");
    json += R"(}},)";
    
    json += R"("name":")" + std::string(app_desc->project_name) + R"(",)";
    json += R"("version":")" + std::string(app_desc->version) + R"(",)";
    json += "\"board_name\":\"" + GetBoardName() + "\",";
    json += R"("compile_time":")" + std::string(app_desc->date) + R"(T)" + std::string(app_desc->time) + R"(Z",)";
    json += R"("idf_version":")" + std::string(app_desc->idf_ver) + R"(",)";
    char sha256_str[65];
    for (int i = 0; i < 32; i++) {
        snprintf(sha256_str + i * 2, sizeof(sha256_str) - i * 2, "%02x", app_desc->app_elf_sha256[i]);
    }
    json += R"("elf_sha256":")" + std::string(sha256_str) + R"(")";
    json += R"(},)";

    json += R"("partition_table": [)";
    esp_partition_iterator_t it = esp_partition_find(ESP_PARTITION_TYPE_ANY, ESP_PARTITION_SUBTYPE_ANY, NULL);
    while (it) {
        const esp_partition_t *partition = esp_partition_get(it);
        json += R"({)";
        json += R"("label":")" + std::string(partition->label) + R"(",)";
        json += R"("type":)" + std::to_string(partition->type) + R"(,)";
        json += R"("subtype":)" + std::to_string(partition->subtype) + R"(,)";
        json += R"("address":)" + std::to_string(partition->address) + R"(,)";
        json += R"("size":)" + std::to_string(partition->size) + R"(},)";;
        it = esp_partition_next(it);
    }
    json.pop_back(); // Remove the last comma
    json += R"(],)";

    json += R"("ota":{)";
    auto ota_partition = esp_ota_get_running_partition();
    json += R"("label":")" + std::string(ota_partition->label) + R"(")";
    json += R"(},)";

    // Append display info
    auto display = GetDisplay();
    if (display) {
        json += R"("display":{)";
        if (dynamic_cast<OledDisplay*>(display)) {
            json += R"("monochrome":)" + std::string("true") + R"(,)";
        } else {
            json += R"("monochrome":)" + std::string("false") + R"(,)";
        }
        json += R"("width":)" + std::to_string(display->width()) + R"(,)";
        json += R"("height":)" + std::to_string(display->height()) + R"(,)";
        json.pop_back(); // Remove the last comma
    }
    json += R"(},)";

    json += R"("board":)" + GetBoardJson();

    // Close the JSON object
    json += R"(})";
    return json;
}

std::string Board::GetBoardName() {
    return SystemInfo::GetMacAddress();
}

void Board::StartBlufiMode(bool blufi) {
    GetDisplay()->ShowNotification(Lang::Strings::ENTERING_WIFI_CONFIG_MODE);

    auto& application = Application::GetInstance();
    application.PlaySound(Lang::Sounds::OGG_BLUFI);
    bool test_mode = application.GetNertcTestMode();
    
    {
        auto app_desc = esp_app_get_description();
        Settings settings("board", true);
        settings.SetString("id",        SystemInfo::GetMacAddress());
        settings.SetString("type",      std::string("yunxin"));
        settings.SetString("version",   esp_app_get_description()->version);
        settings.SetInt("blufi",        blufi ? 1 : 0);
        settings.SetInt("test",         test_mode ? 1 : 0);
        
        std::string app = "app";
        settings.EraseKey(app.c_str());
        settings.SetInt(app.c_str(), 1); //blufi 1, ota 2
        ESP_LOGI(TAG, "StartBlufiMode name:%s id:%s type:%s version:%s set blufi nvs app to 1", 
            GetBoardName().c_str(), SystemInfo::GetMacAddress().c_str(), std::string(app_desc->project_name).c_str(),
            esp_app_get_description()->version);
    }

    const esp_partition_t *blufi_partition = esp_partition_find_first(
        ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_OTA_1, "blufi");
    
    if (blufi_partition != NULL) {
        esp_ota_set_boot_partition(blufi_partition);
        ESP_LOGI(TAG, "Switched to blufi partition:%s at offset 0x%lx subtype:%d, restarting...\n",
            blufi_partition->label, blufi_partition->address, blufi_partition->subtype);

        vTaskDelay(pdMS_TO_TICKS(1000));
        esp_restart();
    } else {
        ESP_LOGI(TAG, "Blufi partition not found!\n");
    }
}

void Board::StartBlufiOtaMode(const std::string& upgrade_url, const std::string& upgrade_version, const std::string& md5) {
    GetDisplay()->ShowNotification(Lang::Strings::OTA_UPGRADE);
    
    {
        Settings settings("board", true);
        
        std::string app = "app";
        settings.EraseKey(app.c_str());
        settings.SetInt(app.c_str(), 2); //blufi 1, ota 2
        settings.SetString("version",   esp_app_get_description()->version);
        settings.EraseKey("ota_v");
        settings.EraseKey("ota_url");
        settings.EraseKey("ota_md5");
        settings.SetString("ota_v", upgrade_version.c_str());
        settings.SetString("ota_url",   upgrade_url.c_str());
        settings.SetString("ota_md5",   md5.c_str()); 
        ESP_LOGI(TAG, "StartBlufiOtaMode %s set blufi nvs app to 2. upgrade_url: %s upgrade_version: %s md5: %s", 
            GetBoardName().c_str(), upgrade_url.c_str(), upgrade_version.c_str(), md5.c_str());
    }

    vTaskDelay(pdMS_TO_TICKS(100));

    { //这个数据非常重要，check nvs 是否写成功
        Settings settings("board", false);
        
        std::string app = "app";
        int appType = settings.GetInt(app.c_str(), 0); //blufi 1, ota 2
        if (appType != 2) {
            ESP_LOGE(TAG, "StartBlufiOtaMode set blufi nvs app failed, appType=%d", appType);
            esp_restart();
        }
        ESP_LOGI(TAG, "StartBlufiOtaMode get blufi nvs app:%d", appType);
    }

    const esp_partition_t *blufi_partition = esp_partition_find_first(
        ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_OTA_1, "blufi");
    
    if (blufi_partition != NULL) {
        esp_ota_set_boot_partition(blufi_partition);
        ESP_LOGI(TAG, "Switched to blufi ota partition:%s at offset 0x%lx subtype:%d, restarting...\n",
            blufi_partition->label, blufi_partition->address, blufi_partition->subtype);

        vTaskDelay(pdMS_TO_TICKS(300));
        esp_restart();
    } else {
        ESP_LOGI(TAG, "Blufi partition not found!\n");
    }
}