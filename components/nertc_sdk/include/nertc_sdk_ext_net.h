#ifndef NETWORK_C_INTERFACE_H
#define NETWORK_C_INTERFACE_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include "nertc_sdk_error.h"

#ifdef __cplusplus
extern "C" {
#endif

// 通用类型定义
typedef void* HttpHandle;
typedef void* TcpHandle;
typedef void* UdpHandle;

// HTTP 接口函数指针类型 全部是同步函数
typedef HttpHandle (*HttpCreateFunc)();
typedef void (*HttpDestroyFunc)(HttpHandle handle);
typedef void (*HttpSetHeaderFunc)(HttpHandle handle, const char* key, const char* value);
typedef bool (*HttpOpenFunc)(HttpHandle handle, const char* method, const char* url, const char* content, size_t length);
typedef void (*HttpCloseFunc)(HttpHandle handle);
typedef int (*HttpGetStatusCodeFunc)(HttpHandle handle);
typedef const char* (*HttpGetResponseHeaderFunc)(HttpHandle handle, const char* key);
typedef size_t (*HttpGetBodyLengthFunc)(HttpHandle handle);
typedef size_t (*HttpGetBodyFunc)(HttpHandle handle, char* buffer, size_t buffer_size);

// TCP 接口函数指针类型
typedef TcpHandle (*TcpCreateFunc)();
typedef void (*TcpSetSocketOpt)(TcpHandle handle, int timeout, int nonblocking);
typedef void (*TcpDestroyFunc)(TcpHandle handle);
typedef bool (*TcpConnectFunc)(TcpHandle handle, const char* host, int port);
typedef void (*TcpDisconnectFunc)(TcpHandle handle);
typedef int (*TcpSendFunc)(TcpHandle handle, const char* data, size_t length);
typedef int (*TcpRecvFunc)(TcpHandle handle, char* buffer, size_t buffer_size);

// UDP 接口函数指针类型
typedef UdpHandle (*UdpCreateFunc)();
typedef void (*UdpSetSocketOpt)(UdpHandle handle, int timeout, int nonblocking);
typedef void (*UdpDestroyFunc)(UdpHandle handle);
typedef bool (*UdpConnectFunc)(UdpHandle handle, const char* host, int port);
typedef void (*UdpDisconnectFunc)(UdpHandle handle);
typedef int (*UdpSendFunc)(UdpHandle handle, const char* data, size_t length);
typedef int (*UdpRecvFunc)(UdpHandle handle, char* buffer, size_t buffer_size);

// 网络接口函数表
typedef struct {
    // HTTP 相关函数指针
    HttpCreateFunc create_http;
    HttpDestroyFunc destroy_http;
    HttpSetHeaderFunc set_header;
    HttpOpenFunc open;
    HttpCloseFunc close;
    HttpGetStatusCodeFunc get_status_code;
    HttpGetResponseHeaderFunc get_response_header;
    HttpGetBodyLengthFunc get_body_length;
    HttpGetBodyFunc get_body;

    // TCP 相关函数指针
    TcpCreateFunc create_tcp;
    TcpSetSocketOpt set_socket_opt_tcp;
    TcpDestroyFunc destroy_tcp;
    TcpConnectFunc connect_tcp;
    TcpDisconnectFunc disconnect_tcp;
    TcpSendFunc send_tcp;
    TcpRecvFunc recv_tcp;

    // UDP 相关函数指针
    UdpCreateFunc create_udp;
    UdpSetSocketOpt set_socket_opt_udp;
    UdpDestroyFunc destroy_udp;
    UdpConnectFunc connect_udp;
    UdpDisconnectFunc disconnect_udp;
    UdpSendFunc send_udp;
    UdpRecvFunc recv_udp;

} NetworkInterfaceVTable;

#ifdef __cplusplus
}
#endif

#endif // NETWORK_C_INTERFACE_H