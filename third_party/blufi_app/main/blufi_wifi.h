#ifndef _BLUFI_WIFI_H_
#define _BLUFI_WIFI_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int succ;
    char ssid[32];      // SSID最大长度32字节（ESP32限制）
    char password[64];  // 密码最大长度64字节（ESP32限制）
} wifi_credential_t;

wifi_credential_t initialise_wifi_and_blufi(const char* prefix);

#ifdef __cplusplus
}
#endif

#endif