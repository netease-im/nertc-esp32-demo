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
    
    NetworkInterfaceVTable* GetVTable() { return &vtable_; }
private:
    NeRtcExternalNetwork();
    ~NeRtcExternalNetwork();

private:
    // HTTP 实现函数
    static HttpHandle CreateHttp();
    static void DestroyHttp(HttpHandle handle);
    static void SetHttpHeader(HttpHandle handle, const char* key, const char* value);
    static bool OpenHttp(HttpHandle handle, const char* method, const char* url, const char* content, size_t length);
    static void CloseHttp(HttpHandle handle);
    static int GetHttpStatusCode(HttpHandle handle);
    static const char* GetHttpResponseHeader(HttpHandle handle, const char* key);
    static size_t GetHttpBodyLength(HttpHandle handle);
    static size_t GetHttpBody(HttpHandle handle, char* buffer, size_t buffer_size);

    // TCP 实现函数
    static TcpHandle CreateTcp();
    static void SetTcpSocketOpt(TcpHandle handle, int timeout, int nonblocking);
    static void DestroyTcp(TcpHandle handle);
    static bool ConnectTcp(TcpHandle handle, const char* host, int port);
    static void DisconnectTcp(TcpHandle handle);
    static int SendTcp(TcpHandle handle, const char* data, size_t length);
    static int RecvTcp(TcpHandle handle, char* buffer, size_t buffer_size);

    // UDP 实现函数
    static UdpHandle CreateUdp();
    static void SetUdpSocketOpt(UdpHandle handle, int timeout, int nonblocking);
    static void DestroyUdp(UdpHandle handle);
    static bool ConnectUdp(UdpHandle handle, const char* host, int port);
    static void DisconnectUdp(UdpHandle handle);
    static int SendUdp(UdpHandle handle, const char* data, size_t length);
    static int RecvUdp(UdpHandle handle, char* buffer, size_t buffer_size);

private:
    static NeRtcExternalNetwork* instance_;
    NetworkInterfaceVTable vtable_;
    EventGroupHandle_t event_group_ = nullptr;
    std::mutex mutex_;
    std::list<std::string> udp_data_queue_; //udp data queue
    int udp_timeout_ms_ = 5000;
};







#endif