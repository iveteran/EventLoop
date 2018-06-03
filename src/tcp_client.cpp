#include "tcp_client.h"
#include "eventloop.h"
#include <unistd.h>
#include <errno.h>
#if defined(__linux__)
#include <linux/tcp.h>
#endif

namespace evt_loop {

bool SetTcpKeepAlive(int fd, bool enable, int idle, int interval, int count)
{
#if defined(__linux__)
    int keepAlive = enable;         // 设定KeepAlive
    int keepIdle = idle;            // 开始首次KeepAlive探测前的TCP空闭时间
    int keepInterval = interval;    // 两次KeepAlive探测间的时间间隔
    int keepCount = count;          // 判定断开前的KeepAlive探测次数
    if (setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, (void*)&keepAlive, sizeof(keepAlive)) == -1)
    {
        return false;
    }
    if (setsockopt(fd, IPPROTO_TCP, TCP_KEEPIDLE, (void *)&keepIdle, sizeof(keepIdle)) == -1)
    {
        return false;
    }
    if (setsockopt(fd, IPPROTO_TCP, TCP_KEEPINTVL, (void *)&keepInterval, sizeof(keepInterval)) == -1)
    {
        return false;
    }
    if (setsockopt(fd, IPPROTO_TCP, TCP_KEEPCNT, (void *)&keepCount, sizeof(keepCount)) == -1)
    {
        return false;
    }
#endif
    return true;
}

TcpClient::TcpClient(const char *host, uint16_t port, MessageType msg_type, bool auto_reconnect, TcpCallbacksPtr tcp_evt_cbs)
    : msg_type_(msg_type), keepalive_(false), auto_reconnect_(auto_reconnect), conn_(nullptr),
    reconnect_timer_(std::bind(&TcpClient::OnReconnectTimer, this, std::placeholders::_1)),
    tcp_evt_cbs_(tcp_evt_cbs)
{
    InitAddress(host, port);

    if (auto_reconnect_) {
        timeval tv;
        tv.tv_sec = 1;
        tv.tv_usec = 0;
        reconnect_timer_.SetInterval(tv);
    }

    //Connect();
}

TcpClient::~TcpClient()
{
    Disconnect();
}

void TcpClient::InitAddress(const char* host, uint16_t port)
{
    server_addr_.port_ = port;
    if (host[0] == '\0' || strcmp(host, "localhost") == 0) {
        server_addr_.ip_ = "127.0.0.1";
    } else if (strcmp(host, "any") == 0) {
        server_addr_.ip_ = "0.0.0.0";
    } else {
        server_addr_.ip_ = host;
    }
}

bool TcpClient::Connect()
{
    bool success = Connect_();
    if (!success && auto_reconnect_)
        Reconnect();
    return success;
}

void TcpClient::Disconnect()
{
    if (conn_) {
        //delete conn_;
        conn_ = nullptr;
    }
}

void TcpClient::Reconnect()
{
    if (!reconnect_timer_.IsRunning())
      reconnect_timer_.Start();
}

bool TcpClient::Send(const string& msg)
{
    bool success = true;
    if (conn_ && conn_->GetState() == BufferIOEvent::READY) {
        conn_->Send(msg);
    } else {
        tmp_sendbuf_list_.push_back(msg);
    }
    return success;
}

void TcpClient::EnableKeepAlive(bool enable)
{
    keepalive_ = enable;
}

void TcpClient::EnableHeartbeat(uint32_t idle_interval, uint32_t ping_interval, uint32_t ping_total)
{
    hb_tmp_params_ = std::make_shared<HeartbeatParams>(idle_interval, ping_interval, ping_total);
    if (conn_) conn_->EnableHeartbeat(idle_interval, ping_interval, ping_total);
}

void TcpClient::SetTcpCallbacks(const TcpCallbacksPtr& tcp_evt_cbs)
{
    tcp_evt_cbs_ = tcp_evt_cbs;
    if (conn_) conn_->SetTcpCallbacks(tcp_evt_cbs_);
}

void TcpClient::SetNewClientCallback(const OnNewClientCallback& new_client_cb)
{
    new_client_cb_ = new_client_cb;
}

void TcpClient::SetErrorCallback(const OnClientErrorCallback& error_cb)
{
    error_cb_ = error_cb;
}

void TcpClient::OnConnected(int fd, const IPAddress& local_addr)
{
    conn_ = CreateClient(fd, local_addr, server_addr_, server_addr_);
    conn_->SetMessageType(msg_type_);
    conn_->AsClient();
    conn_->SetReadyCallback(std::bind(&TcpClient::OnReady, this, std::placeholders::_1));
    if (hb_tmp_params_) {
        conn_->EnableHeartbeat(hb_tmp_params_->idle_interval, hb_tmp_params_->ping_interval, hb_tmp_params_->ping_total);
    }
    if (new_client_cb_) new_client_cb_(conn_.get());
}

void TcpClient::OnConnectionClosed(TcpConnection* conn)
{
    //delete conn_;
    conn_ = nullptr;
    if (auto_reconnect_) {
        Reconnect();
    }
}

bool TcpClient::Connect_()
{
    int fd;
    sockaddr_in sock_addr;

    fd = socket(AF_INET, SOCK_STREAM, 0);
    sock_addr.sin_family = PF_INET;
    sock_addr.sin_port = htons(server_addr_.port_);
    inet_aton(server_addr_.ip_.c_str(), &sock_addr.sin_addr);
    int reuseaddr = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuseaddr, sizeof(reuseaddr)) == -1) {
        OnError(errno, strerror(errno));
        close(fd);
        return false;
    }

    if (keepalive_) {
        bool success = SetTcpKeepAlive(fd, true, 60, 5, 3);
        if (!success) {
            OnError(errno, strerror(errno));
            close(fd);
            return false;
        }
    }

    if (connect(fd, (sockaddr*)&sock_addr, sizeof(sockaddr_in)) == -1) {
        OnError(errno, strerror(errno));
        close(fd);
        return false;
    }
    IPAddress local_addr;
    SocketAddrToIPAddress(sock_addr, local_addr);
    OnConnected(fd, local_addr);

    return true;
}

void TcpClient::OnError(int errcode, const char* errstr)
{
    printf("[TcpClient::OnError] error code: %d, error string: %s\n", errcode, errstr);
    if (error_cb_) error_cb_(this, errcode, errstr);
}

void TcpClient::SendTempBuffer()
{
    printf("[TcpClient::SendTempBuffer] backlogged messages: %ld\n", tmp_sendbuf_list_.size());
    if (conn_ == NULL) return;

    while (!tmp_sendbuf_list_.empty()) {
        const string& sendbuf = tmp_sendbuf_list_.front();
        conn_->Send(sendbuf);
        tmp_sendbuf_list_.pop_front();
    }
}

void TcpClient::OnReconnectTimer(TimerEvent* timer)
{
    if (!conn_) {  // if the connection is not created, then reconnect
        bool success = Connect_();
        if (success) {
            timer->Stop();
        } else {
            printf("[TcpClient::OnReconnectTimer] Reconnect failed, retry %u seconds later...\n", timer->GetInterval().Seconds());
        }
    } else {
        timer->Stop();
    }
}

///////////////////////////////////////////////

TcpClient6::TcpClient6(const char *host, uint16_t port, MessageType msg_type, bool auto_reconnect,
        TcpCallbacksPtr tcp_evt_cbs) : TcpClient(host, port, msg_type, auto_reconnect, tcp_evt_cbs)
{
    /*
    InitAddress(host, port);

    if (auto_reconnect_) {
        timeval tv;
        tv.tv_sec = 1;
        tv.tv_usec = 0;
        reconnect_timer_.SetInterval(tv);
    }
    */

    //Connect();
}

void TcpClient6::InitAddress(const char* host, uint16_t port)
{
    server_addr_.port_ = port;
    if (host[0] == '\0' ||
            strcmp(host, "localhost6") == 0 ||
            strcmp(host, "ip6-localhost") == 0 ||
            strcmp(host, "ipv6-localhost") == 0) {
        server_addr_.ip_ = "::1";
    } else if (strcmp(host, "any") == 0) {
        server_addr_.ip_ = "::";
    } else {
        server_addr_.ip_ = host;
    }
}

bool TcpClient6::Connect_()
{
    int fd;
    sockaddr_in6 sock_addr;
    memset(&sock_addr, 0, sizeof(sock_addr));

    fd = socket(PF_INET6, SOCK_STREAM, 0);
    sock_addr.sin6_family = PF_INET6;
    sock_addr.sin6_port = htons(server_addr_.port_);
    inet_pton(AF_INET6, server_addr_.ip_.c_str(), &sock_addr.sin6_addr);
    int reuseaddr = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuseaddr, sizeof(reuseaddr)) == -1) {
        OnError(errno, strerror(errno));
        close(fd);
        return false;
    }

    if (keepalive_) {
        bool success = SetTcpKeepAlive(fd, true, 60, 5, 3);
        if (!success) {
            OnError(errno, strerror(errno));
            close(fd);
            return false;
        }
    }

    if (connect(fd, (sockaddr*)&sock_addr, sizeof(sock_addr)) == -1) {
        OnError(errno, strerror(errno));
        close(fd);
        return false;
    }
    IPAddress local_addr;
    SocketAddrToIPAddress(sock_addr, local_addr);
    OnConnected(fd, local_addr);

    return true;
}

}  // namespace evt_loop
