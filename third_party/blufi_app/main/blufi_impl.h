/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */


#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_mac.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#if CONFIG_BT_CONTROLLER_ENABLED || !CONFIG_BT_NIMBLE_ENABLED
#include "esp_bt.h"
#endif
#include "esp_blufi_api.h"
#include "esp_blufi.h"
#include "blufi_wifi.h"

#define BLUFI_CONFIG_TAG "BLUFI_CONFIG_WIFI"
#define BLUFI_INFO(fmt, ...)   ESP_LOGI(BLUFI_CONFIG_TAG, fmt, ##__VA_ARGS__)
#define BLUFI_ERROR(fmt, ...)  ESP_LOGE(BLUFI_CONFIG_TAG, fmt, ##__VA_ARGS__)

typedef struct {
    bool sta_connected;
    bool sta_got_ip;
    bool ble_connected;
    bool sta_is_connecting;
    uint8_t sta_bssid[6];
    uint8_t sta_ssid[32];
    int sta_ssid_len;
    wifi_sta_list_t sta_list;
    esp_blufi_extra_info_t sta_conn_info;
    char prefix[26];
    char device_name[32];
    wifi_config_t sta_config;
    wifi_config_t ap_config;
    uint8_t wifi_retry;
    EventGroupHandle_t wifi_result_group;
    EventGroupHandle_t wifi_event_group;
} blufi_context_t;

void blufi_dh_negotiate_data_handler(uint8_t *data, int len, uint8_t **output_data, int *output_len, bool *need_free);
int blufi_aes_encrypt(uint8_t iv8, uint8_t *crypt_data, int crypt_len);
int blufi_aes_decrypt(uint8_t iv8, uint8_t *crypt_data, int crypt_len);
uint16_t blufi_crc_checksum(uint8_t iv8, uint8_t *data, int len);

int blufi_security_init(void);
void blufi_security_deinit(void);
int esp_blufi_gap_register_callback(void);
esp_err_t esp_blufi_host_init(void);
esp_err_t esp_blufi_host_and_cb_init(esp_blufi_callbacks_t *callbacks);
esp_err_t esp_blufi_host_deinit(void);
esp_err_t esp_blufi_controller_init(void);
esp_err_t esp_blufi_controller_deinit(void);
