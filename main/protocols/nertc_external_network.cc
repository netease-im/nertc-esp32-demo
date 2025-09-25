#include <cstring>
#include "nertc_external_network.h"
#include "board.h"
#include <esp_log.h>

#define TAG "NeRtcExternalNetwork"

#define TCP_RECV_EVENT (1 << 0)
#define UDP_RECV_EVENT (1 << 1)

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
    handle_ = {
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

    tcp_event_group_ = xEventGroupCreate();
    udp_event_group_ = xEventGroupCreate();

    ESP_LOGI(TAG, "Create NeRtcExternalNetwork instance");
}

NeRtcExternalNetwork::~NeRtcExternalNetwork() {
    vEventGroupDelete(udp_event_group_);
    vEventGroupDelete(tcp_event_group_);
}

// HTTP 实现
http_handle NeRtcExternalNetwork::CreateHttp() {
    auto network = Board::GetInstance().GetNetwork();
    auto http_unique = network->CreateHttp(0);
    Http* http = http_unique.release();

    return static_cast<void*>(http);
}

void NeRtcExternalNetwork::DestroyHttp(http_handle handle) {
    if (!handle)
        return;

    Http* http = static_cast<Http*>(handle);
    delete http;
}

void NeRtcExternalNetwork::SetHttpHeader(http_handle handle, const char* key, const char* value) {
    if (!handle)
        return;
        
    Http* http = static_cast<Http*>(handle);
    http->SetHeader(key, value);
}

bool NeRtcExternalNetwork::OpenHttp(http_handle handle, const char* method, const char* url, const char* content, size_t length) {
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

void NeRtcExternalNetwork::CloseHttp(http_handle handle) {
    if (!handle)
        return;

    Http* http = static_cast<Http*>(handle);
    http->Close();
}

int NeRtcExternalNetwork::GetHttpStatusCode(http_handle handle) {
    if (!handle)
        return -1;

    Http* http = static_cast<Http*>(handle);
    return http->GetStatusCode();
}

const char* NeRtcExternalNetwork::GetHttpResponseHeader(http_handle handle, const char* key) {
    if (!handle)
        return "";

    Http* http = static_cast<Http*>(handle);
    return http->GetResponseHeader(key).c_str();
}

size_t NeRtcExternalNetwork::GetHttpBodyLength(http_handle handle) {
    if (!handle)
        return 0;

    Http* http = static_cast<Http*>(handle);
    return http->GetBodyLength();
}

size_t NeRtcExternalNetwork::GetHttpBody(http_handle handle, char* buffer, size_t buffer_size) {
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

tcp_handle NeRtcExternalNetwork::CreateTcp() {
    auto network = Board::GetInstance().GetNetwork();
    auto tcp_unique = network->CreateTcp(1);
    Tcp* tcp = tcp_unique.release();

    tcp->OnStream([](const std::string& data) {
        NeRtcExternalNetwork* ext_net = NeRtcExternalNetwork::GetInstance();
        std::lock_guard<std::mutex> lock(ext_net->tcp_mutex_);
        ext_net->tcp_recv_buffer_.append(data);
        xEventGroupSetBits(ext_net->tcp_event_group_, TCP_RECV_EVENT);
    });

    return static_cast<void*>(tcp);
}

void NeRtcExternalNetwork::SetTcpSocketOpt(tcp_handle, int, int) {

}

void NeRtcExternalNetwork::DestroyTcp(tcp_handle handle) {
    if (!handle)
        return;

    Tcp* tcp = static_cast<Tcp*>(handle);
    delete tcp;
}

bool NeRtcExternalNetwork::ConnectTcp(tcp_handle handle, const char* host, int port) {
    if (!handle)
        return false;
        
    Tcp* tcp = static_cast<Tcp*>(handle);
    return tcp->Connect(host, port);
}

void NeRtcExternalNetwork::DisconnectTcp(tcp_handle handle) {
    if (!handle)
        return;

    Tcp* tcp = static_cast<Tcp*>(handle);
    tcp->Disconnect();
}

int NeRtcExternalNetwork::SendTcp(tcp_handle handle, const char* data, size_t length) {
    if (!handle)
        return -1;

    Tcp* tcp = static_cast<Tcp*>(handle);
    return tcp->Send(std::string(data, length));
}

int NeRtcExternalNetwork::RecvTcp(tcp_handle handle,
                                  char* buffer,
                                  size_t buffer_size) {
    if (!handle || !buffer)
        return -1;

    NeRtcExternalNetwork* ext_net = NeRtcExternalNetwork::GetInstance();

    {
        std::lock_guard<std::mutex> lock(ext_net->tcp_mutex_);
        if (!ext_net->tcp_recv_buffer_.empty()) {
            size_t to_copy = std::min(buffer_size, ext_net->tcp_recv_buffer_.size());
            memcpy(buffer, ext_net->tcp_recv_buffer_.data(), to_copy);
            ext_net->tcp_recv_buffer_.erase(0, to_copy);
            return static_cast<int>(to_copy);   // 立刻返回，不阻塞
        }
    }

    EventBits_t bits = xEventGroupWaitBits(ext_net->tcp_event_group_,
                                           TCP_RECV_EVENT,
                                           pdTRUE,          // 退出时清除事件
                                           pdFALSE,
                                           portMAX_DELAY);
    if ((bits & TCP_RECV_EVENT) == 0)
        return 0;  

    std::lock_guard<std::mutex> lock(ext_net->tcp_mutex_);
    size_t to_copy = std::min(buffer_size, ext_net->tcp_recv_buffer_.size());
    memcpy(buffer, ext_net->tcp_recv_buffer_.data(), to_copy);
    ext_net->tcp_recv_buffer_.erase(0, to_copy);
    return static_cast<int>(to_copy);
}

udp_handle NeRtcExternalNetwork::CreateUdp() {
    auto network = Board::GetInstance().GetNetwork();
    auto udp_unique = network->CreateUdp(2);
    Udp* udp = udp_unique.release();

    udp->OnMessage([](const std::string& data) {
        NeRtcExternalNetwork* ext_net = NeRtcExternalNetwork::GetInstance();
        std::lock_guard<std::mutex> lock(ext_net->udp_mutex_);
        ext_net->udp_data_queue_.emplace_back(data);
        xEventGroupSetBits(ext_net->udp_event_group_, UDP_RECV_EVENT);
    });

    return static_cast<void*>(udp);
}

void NeRtcExternalNetwork::SetUdpSocketOpt(udp_handle, int timeout, int) {
    NeRtcExternalNetwork* ext_net = NeRtcExternalNetwork::GetInstance();
    ext_net->udp_timeout_ms_ = timeout;
}

void NeRtcExternalNetwork::DestroyUdp(udp_handle handle) {
    if (!handle)
        return;

    Udp* udp = static_cast<Udp*>(handle);
    delete udp;
}

bool NeRtcExternalNetwork::ConnectUdp(udp_handle handle, const char* host, int port) {
    if (!handle)
        return false;
        
    Udp* udp = static_cast<Udp*>(handle);
    return udp->Connect(std::string(host), (int)port);
}

void NeRtcExternalNetwork::DisconnectUdp(udp_handle handle) {
    if (!handle)
        return;

    Udp* udp = static_cast<Udp*>(handle);
    udp->Disconnect();
}

int NeRtcExternalNetwork::SendUdp(udp_handle handle, const char* data, size_t length) {
    if (!handle)
        return -1;

    Udp* udp = static_cast<Udp*>(handle);
    return udp->Send(std::string(data, length));
}

int NeRtcExternalNetwork::RecvUdp(udp_handle handle, char* buffer, size_t buffer_size) {
    if (!handle)
        return -1;

    NeRtcExternalNetwork* ext_net = NeRtcExternalNetwork::GetInstance();
    auto bits = xEventGroupWaitBits(ext_net->udp_event_group_, UDP_RECV_EVENT, pdTRUE, pdFALSE, pdMS_TO_TICKS(ext_net->udp_timeout_ms_));
    if (bits & UDP_RECV_EVENT) {
        std::lock_guard<std::mutex> lock(ext_net->udp_mutex_);
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