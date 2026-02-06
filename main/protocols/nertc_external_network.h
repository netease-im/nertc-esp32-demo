#ifndef _NERTC_EXTERN_NETWORK_H_
#define _NERTC_EXTERN_NETWORK_H_

#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <list>
#include <mutex>
#include <string>
#include "nertc_sdk_ext_net.h"

class NeRtcExternalNetwork {
public:
    static NeRtcExternalNetwork* GetInstance();
    static void DestroyInstance();
    
    nertc_sdk_ext_net_handle_t* GetHandle() { return &handle_; }
private:
    NeRtcExternalNetwork();
    ~NeRtcExternalNetwork();

private:
    // HTTP 实现函数
    static http_handle CreateHttp();
    static void DestroyHttp(http_handle handle);
    static void SetHttpHeader(http_handle handle, const char* key, const char* value);
    static bool OpenHttp(http_handle handle, const char* method, const char* url, const char* content, size_t length);
    static void CloseHttp(http_handle handle);
    static int GetHttpStatusCode(http_handle handle);
    static const char* GetHttpResponseHeader(http_handle handle, const char* key);
    static size_t GetHttpBodyLength(http_handle handle);
    static size_t GetHttpBody(http_handle handle, char* buffer, size_t buffer_size);

    // TCP 实现函数
    static tcp_handle CreateTcp();
    static void SetTcpSocketOpt(tcp_handle handle, int timeout, int nonblocking);
    static void DestroyTcp(tcp_handle handle);
    static bool ConnectTcp(tcp_handle handle, const char* host, int port);
    static void DisconnectTcp(tcp_handle handle);
    static int SendTcp(tcp_handle handle, const char* data, size_t length);
    static int RecvTcp(tcp_handle handle, char* buffer, size_t buffer_size);

    // UDP 实现函数
    static udp_handle CreateUdp();
    static void SetUdpSocketOpt(udp_handle handle, int timeout, int nonblocking);
    static void DestroyUdp(udp_handle handle);
    static bool ConnectUdp(udp_handle handle, const char* host, int port);
    static void DisconnectUdp(udp_handle handle);
    static int SendUdp(udp_handle handle, const char* data, size_t length);
    static int RecvUdp(udp_handle handle, char* buffer, size_t buffer_size);

    // MQTT 实现函数
    static mqtt_handle CreateMqtt();
    static void DestroyMqtt(mqtt_handle handle);
    static void SetMqttOnConnected(mqtt_handle handle, mqtt_on_connected_func callback);
    static void SetMqttOnDisconnected(mqtt_handle handle, mqtt_on_disconnected_func callback);
    static void SetMqttOnMessage(mqtt_handle handle, mqtt_on_message_func callback);
    static void SetMqttOnError(mqtt_handle handle, mqtt_on_error_func callback);
    static bool MqttConnect(mqtt_handle handle, const char* host, int port, const char* client_id, const char* username, const char* password);
    static void MqttDisconnect(mqtt_handle handle);
    static bool MqttIsConnected(mqtt_handle handle);
    static bool MqttPublish(mqtt_handle handle, const char* topic, const char* payload, int qos);
    static bool MqttSubscribe(mqtt_handle handle, const char* topic, int qos);
    static bool MqttUnsubscribe(mqtt_handle handle, const char* topic);

private:
    static NeRtcExternalNetwork* instance_;
    nertc_sdk_ext_net_handle_t handle_;
    EventGroupHandle_t tcp_event_group_ = nullptr;
    EventGroupHandle_t udp_event_group_ = nullptr;
    std::mutex tcp_mutex_;
    std::mutex udp_mutex_;
    std::string tcp_recv_buffer_;
    std::list<std::string> udp_data_queue_;
    int udp_timeout_ms_ = 5000;
};







#endif