#ifndef __NERTC_SDK_EXT_NET_H__
#define __NERTC_SDK_EXT_NET_H__

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "nertc_sdk_error.h"

#ifdef __cplusplus
extern "C" {
#endif

// 通用类型定义
typedef void* http_handle;
typedef void* tcp_handle;
typedef void* udp_handle;
typedef void* mqtt_handle;

// 回调函数类型定义
typedef void (*mqtt_on_connected_func)(mqtt_handle handle);
typedef void (*mqtt_on_disconnected_func)(mqtt_handle handle);
typedef void (*mqtt_on_message_func)(mqtt_handle handle, const char* topic, const char* payload);
typedef void (*mqtt_on_error_func)(mqtt_handle handle, int, const char* error);

// HTTP 接口函数指针类型 全部是同步函数
typedef http_handle (*http_create_func)();
typedef void (*http_destroy_func)(http_handle handle);
typedef void (*http_set_header_func)(http_handle handle, const char* key, const char* value);
typedef bool (*http_open_func)(http_handle handle, const char* method, const char* url, const char* content, size_t length);
typedef void (*http_close_func)(http_handle handle);
typedef int (*http_get_status_code_func)(http_handle handle);
typedef const char* (*http_get_response_header_func)(http_handle handle, const char* key);
typedef size_t (*http_get_body_length_func)(http_handle handle);
typedef size_t (*http_get_body_func)(http_handle handle, char* buffer, size_t buffer_size);

// MQTT 接口函数指针类型 - 异步函数
typedef mqtt_handle (*mqtt_create_func)();
typedef void (*mqtt_destroy_func)(mqtt_handle handle);
typedef void (*mqtt_set_on_connected_func)(mqtt_handle handle, mqtt_on_connected_func callback);
typedef void (*mqtt_set_on_disconnected_func)(mqtt_handle handle, mqtt_on_disconnected_func callback);
typedef void (*mqtt_set_on_message_func)(mqtt_handle handle, mqtt_on_message_func callback);
typedef void (*mqtt_set_on_error_func)(mqtt_handle handle, mqtt_on_error_func callback);
typedef bool (*mqtt_connect_func)(mqtt_handle handle, const char* host, int port, const char* client_id, const char* username, const char* password);
typedef void (*mqtt_disconnect_func)(mqtt_handle handle);
typedef bool (*mqtt_is_connected_func)(mqtt_handle handle);
typedef bool (*mqtt_publish_func)(mqtt_handle handle, const char* topic, const char* payload, int qos);
typedef bool (*mqtt_subscribe_func)(mqtt_handle handle, const char* topic, int qos);
typedef bool (*mqtt_unsubscribe_func)(mqtt_handle handle, const char* topic);

// TCP 接口函数指针类型
typedef tcp_handle (*tcp_create_func)();
typedef void (*tcp_set_socket_opt_func)(tcp_handle handle, int timeout, int nonblocking);
typedef void (*tcp_destroy_func)(tcp_handle handle);
typedef bool (*tcp_connect_func)(tcp_handle handle, const char* host, int port);
typedef void (*tcp_disconnect_func)(tcp_handle handle);
typedef int (*tcp_send_func)(tcp_handle handle, const char* data, size_t length);
typedef int (*tcp_recv_func)(tcp_handle handle, char* buffer, size_t buffer_size);

// UDP 接口函数指针类型
typedef udp_handle (*udp_create_func)();
typedef void (*udp_set_socket_opt_func)(udp_handle handle, int timeout, int nonblocking);
typedef void (*udp_destroy_func)(udp_handle handle);
typedef bool (*udp_connect_func)(udp_handle handle, const char* host, int port);
typedef void (*udp_disconnect_func)(udp_handle handle);
typedef int (*udp_send_func)(udp_handle handle, const char* data, size_t length);
typedef int (*udp_recv_func)(udp_handle handle, char* buffer, size_t buffer_size);

// 网络接口函数表
typedef struct {
  // HTTP 相关函数指针
  http_create_func create_http;
  http_destroy_func destroy_http;
  http_set_header_func set_header;
  http_open_func open;
  http_close_func close;
  http_get_status_code_func get_status_code;
  http_get_response_header_func get_response_header;
  http_get_body_length_func get_body_length;
  http_get_body_func get_body;

  // TCP 相关函数指针
  tcp_create_func create_tcp;
  tcp_set_socket_opt_func set_socket_opt_tcp;
  tcp_destroy_func destroy_tcp;
  tcp_connect_func connect_tcp;
  tcp_disconnect_func disconnect_tcp;
  tcp_send_func send_tcp;
  tcp_recv_func recv_tcp;

  // UDP 相关函数指针
  udp_create_func create_udp;
  udp_set_socket_opt_func set_socket_opt_udp;
  udp_destroy_func destroy_udp;
  udp_connect_func connect_udp;
  udp_disconnect_func disconnect_udp;
  udp_send_func send_udp;
  udp_recv_func recv_udp;

  // MQTT 相关函数指针
  mqtt_create_func create_mqtt;
  mqtt_destroy_func destroy_mqtt;
  mqtt_set_on_connected_func set_mqtt_on_connected;
  mqtt_set_on_disconnected_func set_mqtt_on_disconnected;
  mqtt_set_on_message_func set_mqtt_on_message;
  mqtt_set_on_error_func set_mqtt_on_error;
  mqtt_connect_func mqtt_connect;
  mqtt_disconnect_func mqtt_disconnect;
  mqtt_is_connected_func mqtt_is_connected;
  mqtt_publish_func mqtt_publish;
  mqtt_subscribe_func mqtt_subscribe;
  mqtt_unsubscribe_func mqtt_unsubscribe;

} nertc_sdk_ext_net_handle_t;

#ifdef __cplusplus
}
#endif

#endif  // __NERTC_SDK_EXT_NET_H__
