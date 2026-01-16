#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include <esp_mac.h>
#include <esp_system.h>
#include <esp_ota_ops.h>
#include <esp_partition.h>
#include <nvs_flash.h>
#include "blufi_wifi.h"
#include "ssid_manager.h"
static const char *TAG = "BulfiApp";

extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "Starting BLUFI WiFi configuration...");
    /* 1. 初始化 NVS */
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
        ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }

    std::string broad_name;
    // read nvs setting broad name
    nvs_handle_t nvs_handle = 0;
    nvs_open("board", NVS_READONLY, &nvs_handle);
    if (nvs_handle != 0) {
        size_t length = 0;
        if (nvs_get_str(nvs_handle, "name", nullptr, &length) == ESP_OK) {
            broad_name.resize(length);
            if (nvs_get_str(nvs_handle, "name", broad_name.data(), &length) == ESP_OK) {
                while (!broad_name.empty() && broad_name.back() == '\0') {
                    broad_name.pop_back();
                }
                ESP_LOGI(TAG, "Read board name from nvs: %s", broad_name.c_str());
            } else {
                broad_name.clear();
            }
        }
    }
    
    if (broad_name.empty()) {
        uint8_t mac[6];
    #if CONFIG_IDF_TARGET_ESP32P4
        esp_wifi_get_mac(WIFI_IF_AP, mac);
    #else
        ESP_ERROR_CHECK(esp_read_mac(mac, ESP_MAC_WIFI_SOFTAP));
    #endif
        char ssid[32];
        snprintf(ssid, sizeof(ssid), "%s-%02X%02X", "yunxin", mac[4], mac[5]);
        broad_name = std::string(ssid);
    }
    ESP_LOGI(TAG, "Broad name: %s", broad_name.c_str());

    /* 2. 启动 Wi-Fi + Blufi */
    wifi_credential_t wifi_cred = initialise_wifi_and_blufi(broad_name.c_str());

    if (wifi_cred.succ == 1) {
        ESP_LOGI(TAG, "BLUFI WiFi connected! SSID: %s", wifi_cred.ssid);
        SsidManager::GetInstance().AddSsid(wifi_cred.ssid, wifi_cred.password); //存储账号密码

        /* 设置下一次启动分区 */
        const esp_partition_t *next_ota = esp_ota_get_next_update_partition(NULL);
        ESP_ERROR_CHECK( esp_ota_set_boot_partition(next_ota) ); 
    } else {
        ESP_LOGE(TAG, "BLUFI WiFi connection failed");
    }

    ESP_LOGI(TAG, "BLUFI WiFi configuration completed, restarting...");
    vTaskDelay(pdMS_TO_TICKS(500));
    // 执行重启
    esp_restart();
}