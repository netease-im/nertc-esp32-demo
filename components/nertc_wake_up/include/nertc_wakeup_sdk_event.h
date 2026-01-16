#ifndef __NERTC_WAKE_UP_EVENT_H__
#define __NERTC_WAKE_UP_EVENT_H__


#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void * nertc_wakeup_sdk_t;
typedef void * nertc_wakeup_sdk_user_data_t;

typedef struct nertc_wakeup_sdk_callback_context {
  nertc_wakeup_sdk_t wakeup;        /**< wakeup 实例 */
  nertc_wakeup_sdk_user_data_t user_data;  /**< 用户数据 */
} nertc_wakeup_sdk_callback_context_t;

typedef struct nertc_wakeup_sdk_event_handler {
/**
  *  检测到唤醒词的回调。
  * <br>该回调方法表示 SDK 检测到了唤醒词
  * 干预或提示用户。
  * @param ctx 回调上下文
  * @param wake_word 对应的唤醒词
  * @endif
  */
void (*on_wake_word_detected)(const nertc_wakeup_sdk_callback_context_t* ctx, const char *wake_word);
} nertc_wakeup_sdk_event_handle_t;

//外部http io接口定义
typedef void* http_handle;
typedef http_handle (*http_create_func)();
typedef void (*http_destroy_func)(http_handle handle);
typedef void (*http_set_header_func)(http_handle handle, const char* key, const char* value);
typedef bool (*http_open_func)(http_handle handle, const char* method, const char* url, const char* content, size_t length);
typedef void (*http_close_func)(http_handle handle);
typedef int (*http_get_status_code_func)(http_handle handle);
typedef size_t (*http_get_body_length_func)(http_handle handle);
typedef size_t (*http_get_body_func)(http_handle handle, char* buffer, size_t buffer_size);

typedef struct {
  http_create_func create_http;
  http_destroy_func destroy_http;
  http_set_header_func set_header;
  http_open_func open;
  http_close_func close;
  http_get_status_code_func get_status_code;
  http_get_body_length_func get_body_length;
  http_get_body_func get_body;
} nertc_wakeup_sdk_ext_http_io_t;

typedef struct nertc_wakeup_sdk_config {
  nertc_wakeup_sdk_event_handle_t event_handler;
  nertc_wakeup_sdk_user_data_t user_data;
  const char* appkey;
  const char* deviceId;
  const char* custom_config;
  void* models_list;
  nertc_wakeup_sdk_ext_http_io_t *http_io; //外部http io接口, 4G板子使用
} nertc_wakeup_sdk_config_t;

#ifdef __cplusplus
}
#endif

#endif // __NERTC_SDK_EVENT_H__