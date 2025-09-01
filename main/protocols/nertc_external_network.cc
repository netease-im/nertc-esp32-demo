#include <cstring>
#include "nertc_external_network.h"
#include "board.h"
#include "ml307_ssl_transport.h"
#include <esp_log.h>

#define TAG "NeRtcExternalNetwork"

#define UDP_RECV_EVENT (1 << 0)

NeRtcExternalNetwork* NeRtcExternalNetwork::instance_ = nullptr;
NeRtcExternalNetwork* NeRtcExternalNetwork::GetInstance() {
  if (instance_ == nullptr) {
    instance_ = new NeRtcExternalNetwork();
  }
  return instance_;
}

void NeRtcExternalNetwork::DestroyInstance() {
  delete instance_;
  instance_ = nullptr;
}

NeRtcExternalNetwork::NeRtcExternalNetwork() {
    // 初始化函数表
    vtable_ = {
        // HTTP 函数指针
        .create_http = CreateHttp,
        .destroy_http = DestroyHttp,
        .set_header = SetHttpHeader,
        .open = OpenHttp,
        .close = CloseHttp,
        .get_status_code = GetHttpStatusCode,
        .get_response_header = GetHttpResponseHeader,
        .get_body_length = GetHttpBodyLength,
        .get_body = GetHttpBody,

        // TCP 函数指针
        .create_tcp = CreateTcp,
        .set_socket_opt_tcp = SetTcpSocketOpt,
        .destroy_tcp = DestroyTcp,
        .connect_tcp = ConnectTcp,
        .disconnect_tcp = DisconnectTcp,
        .send_tcp = SendTcp,
        .recv_tcp = RecvTcp,

        // UDP 函数指针
        .create_udp = CreateUdp,
        .set_socket_opt_udp = SetUdpSocketOpt,
        .destroy_udp = DestroyUdp,
        .connect_udp = ConnectUdp,
        .disconnect_udp = DisconnectUdp,
        .send_udp = SendUdp,
        .recv_udp = RecvUdp
    };

    event_group_ = xEventGroupCreate();
}

NeRtcExternalNetwork::~NeRtcExternalNetwork() {
    vEventGroupDelete(event_group_);
}

// HTTP 实现
HttpHandle NeRtcExternalNetwork::CreateHttp() {
    auto http = Board::GetInstance().CreateHttp();

    return static_cast<void*>(http);
}

void NeRtcExternalNetwork::DestroyHttp(HttpHandle handle) {
    if (!handle)
        return;

    Http* http = static_cast<Http*>(handle);
    delete http;
}

void NeRtcExternalNetwork::SetHttpHeader(HttpHandle handle, const char* key, const char* value) {
    if (!handle)
        return;
        
    Http* http = static_cast<Http*>(handle);
    http->SetHeader(key, value);
}

bool NeRtcExternalNetwork::OpenHttp(HttpHandle handle, const char* method, const char* url, const char* content, size_t length) {
    if (!handle)
        return false;
        
    Http* http = static_cast<Http*>(handle);
    http->SetContent(std::string(content, length));
    if (!http->Open(method, url)) {
        ESP_LOGE(TAG, "Failed to open HTTP connection. url: %s", url);    
        return false;
    }

    return true;
}

void NeRtcExternalNetwork::CloseHttp(HttpHandle handle) {
    if (!handle)
        return;

    Http* http = static_cast<Http*>(handle);
    http->Close();
}

int NeRtcExternalNetwork::GetHttpStatusCode(HttpHandle handle) {
    if (!handle)
        return -1;

    Http* http = static_cast<Http*>(handle);
    return http->GetStatusCode();
}

const char* NeRtcExternalNetwork::GetHttpResponseHeader(HttpHandle handle, const char* key) {
    if (!handle)
        return "";

    Http* http = static_cast<Http*>(handle);
    return http->GetResponseHeader(key).c_str();
}

size_t NeRtcExternalNetwork::GetHttpBodyLength(HttpHandle handle) {
    if (!handle)
        return 0;

    Http* http = static_cast<Http*>(handle);
    return http->GetBodyLength();
}

size_t NeRtcExternalNetwork::GetHttpBody(HttpHandle handle, char* buffer, size_t buffer_size) {
    if (!handle)
        return 0;

    Http* http = static_cast<Http*>(handle);
    size_t body_length = http->GetBodyLength();
    if (buffer_size < body_length) {
        ESP_LOGE(TAG, "Buffer size too small for HTTP body");
        return 0;
    }
    memcpy(buffer, http->ReadAll().c_str(), body_length);
    return body_length;
}

TcpHandle NeRtcExternalNetwork::CreateTcp() {
    auto tcp = Board::GetInstance().CreateTcp(false);

    return static_cast<void*>(tcp);
}

void NeRtcExternalNetwork::SetTcpSocketOpt(TcpHandle, int, int) {

}

void NeRtcExternalNetwork::DestroyTcp(TcpHandle handle) {
    if (!handle)
        return;

    Transport* tcp = static_cast<Transport*>(handle);
    delete tcp;
}

bool NeRtcExternalNetwork::ConnectTcp(TcpHandle handle, const char* host, int port) {
    if (!handle)
        return false;
        
    Transport* tcp = static_cast<Transport*>(handle);
    return tcp->Connect(host, port);
}

void NeRtcExternalNetwork::DisconnectTcp(TcpHandle handle) {
    if (!handle)
        return;

    Transport* tcp = static_cast<Transport*>(handle);
    tcp->Disconnect();
}

int NeRtcExternalNetwork::SendTcp(TcpHandle handle, const char* data, size_t length) {
    if (!handle)
        return -1;

    Transport* tcp = static_cast<Transport*>(handle);
    return tcp->Send(data, length);
}

int NeRtcExternalNetwork::RecvTcp(TcpHandle handle, char* buffer, size_t buffer_size) {
    if (!handle)
        return -1;

    Transport* tcp = static_cast<Transport*>(handle);
    return tcp->Receive(buffer, buffer_size);
}

UdpHandle NeRtcExternalNetwork::CreateUdp() {
    auto udp = Board::GetInstance().CreateUdp();

    udp->OnMessage([](const std::string& data) {
        NeRtcExternalNetwork* ext_net = NeRtcExternalNetwork::GetInstance();
        std::lock_guard<std::mutex> lock(ext_net->mutex_);
        ext_net->udp_data_queue_.emplace_back(data);
        xEventGroupSetBits(ext_net->event_group_, UDP_RECV_EVENT);
    });

    return static_cast<void*>(udp);
}

void NeRtcExternalNetwork::SetUdpSocketOpt(UdpHandle, int timeout, int) {
    NeRtcExternalNetwork* ext_net = NeRtcExternalNetwork::GetInstance();
    ext_net->udp_timeout_ms_ = timeout;
}

void NeRtcExternalNetwork::DestroyUdp(UdpHandle handle) {
    if (!handle)
        return;

    Udp* udp = static_cast<Udp*>(handle);
    delete udp;
}

bool NeRtcExternalNetwork::ConnectUdp(UdpHandle handle, const char* host, int port) {
    if (!handle)
        return false;
        
    Udp* udp = static_cast<Udp*>(handle);
    return udp->Connect(std::string(host), (int)port);
}

void NeRtcExternalNetwork::DisconnectUdp(UdpHandle handle) {
    if (!handle)
        return;

    Udp* udp = static_cast<Udp*>(handle);
    udp->Disconnect();
}

int NeRtcExternalNetwork::SendUdp(UdpHandle handle, const char* data, size_t length) {
    if (!handle)
        return -1;

    Udp* udp = static_cast<Udp*>(handle);
    return udp->Send(std::string(data, length));
}

int NeRtcExternalNetwork::RecvUdp(UdpHandle handle, char* buffer, size_t buffer_size) {
    if (!handle)
        return -1;

    NeRtcExternalNetwork* ext_net = NeRtcExternalNetwork::GetInstance();
    auto bits = xEventGroupWaitBits(ext_net->event_group_, UDP_RECV_EVENT, pdTRUE, pdFALSE, pdMS_TO_TICKS(ext_net->udp_timeout_ms_));
    if (bits & UDP_RECV_EVENT) {
        std::lock_guard<std::mutex> lock(ext_net->mutex_);
        if (!ext_net->udp_data_queue_.empty()) {
            auto& data = ext_net->udp_data_queue_.front();
            size_t to_copy = std::min(buffer_size, data.size());
            memcpy(buffer, data.data(), to_copy);
            ext_net->udp_data_queue_.pop_front();
            return to_copy;
        } else {
            return 0;
        }
    } else {
        // Timeout
        return 0;
    }
}