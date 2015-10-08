#include "tcp_client.h"

namespace evt_loop {

TcpClient::TcpClient(const char *host, uint16_t port, bool auto_reconnect, ITcpEventHandler* tcp_evt_handler)
    : auto_reconnect_(auto_reconnect), conn_(NULL), reconnect_timer_(this), tcp_evt_handler_(tcp_evt_handler)
{
    server_addr_.port_ = port;
    if (host[0] == '\0' || strcmp(host, "localhost") == 0) {
        server_addr_.ip_ = "127.0.0.1";
    } else if (!strcmp(host, "any")) {
        server_addr_.ip_ = "0.0.0.0";
    } else {
        server_addr_.ip_ = host;
    }

    timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    reconnect_timer_.SetInterval(tv);
    EV_Singleton->AddEvent(&reconnect_timer_);

    EV_Singleton->AddEvent(this);
    Connect();
}

TcpClient::~TcpClient()
{
    Disconnect();
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
        delete conn_;
        conn_ = NULL;
    }
}

void TcpClient::Reconnect()
{
    reconnect_timer_.Start();
}

bool TcpClient::Send(const string& msg)
{
    bool success = true;
    if (conn_) {
        conn_->Send(msg);
    } else {
        tmp_sendbuf_list_.push_back(msg);
    }
    return success;
}

void TcpClient::SetTcpEventHandler(ITcpEventHandler* evt_handler)
{
    tcp_evt_handler_ = evt_handler;
    if (conn_) conn_->SetTcpEventHandler(tcp_evt_handler_);
}

void TcpClient::OnConnected(int fd, const IPAddress& local_addr)
{
    conn_ = new TcpConnection(fd, local_addr, server_addr_, tcp_evt_handler_, this);
    SendTempBuffer();
    if (tcp_evt_handler_) tcp_evt_handler_->OnNewConnection(conn_);
}

void TcpClient::OnConnectionClosed(TcpConnection* conn)
{
    delete conn_;
    conn_ = NULL;
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
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuseaddr, sizeof(reuseaddr)) == -1)
    {
        OnError(errno, strerror(errno));
        close(fd);
        return false;
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
    if (tcp_evt_handler_) tcp_evt_handler_->OnError(errcode, errstr);
}

void TcpClient::SendTempBuffer()
{
    if (conn_ == NULL) return;

    while (!tmp_sendbuf_list_.empty()) {
        const string& sendbuf = tmp_sendbuf_list_.front();
        conn_->Send(sendbuf);
        tmp_sendbuf_list_.pop_front();
    }
}

void TcpClient::ReconnectTimer::OnTimer()
{
    if (!creator_->conn_)
    {
        bool success = creator_->Connect_();
        if (success)
            creator_->reconnect_timer_.Stop();
        else
            printf("Reconnect failed, retry %ld seconds later...\n", GetInterval().tv_sec);
    } else {
        creator_->reconnect_timer_.Stop();
    }
}

}  // namespace evt_loop
