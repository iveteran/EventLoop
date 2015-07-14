#include "tcp_client.h"

namespace richinfo {

TcpClient::TcpClient(const char *host, uint16_t port, bool auto_reconnect)
    : auto_reconnect_(auto_reconnect), conn_(NULL), reconnect_timer_(this)
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

bool TcpClient::Connect()
{
    bool success = Connect_();
    if (!success && auto_reconnect_)
        Reconnect();
    return success;
}

bool TcpClient::Send(const string& msg)
{
    if (conn_) {
        conn_->Send(msg);
        return true;
    } else {
        printf("[TcpClient::Send] Error: connection not ready\n");
        return false;
    }
}

void TcpClient::SetOnMessageCb(const OnMessageCallback& on_msg_cb)
{
    on_msg_cb_ = on_msg_cb;
    if (conn_) conn_->SetOnMessageCb(on_msg_cb_);
}

void TcpClient::OnConnected(int fd, const IPAddress& local_addr)
{
    conn_ = new TcpConnection(fd, local_addr, server_addr_, this);
    conn_->SetOnMessageCb(on_msg_cb_);
}

void TcpClient::OnConnectionClosed(TcpConnection* conn)
{
    EV_Singleton->DeleteEvent(conn);
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

    if (connect(fd, (sockaddr*)&sock_addr, sizeof(sockaddr_in)) == -1) {
        printf("Connect failed: %s\n", strerror(errno));
        close(fd);
        return false;
    }
    IPAddress local_addr;
    SocketAddrToIPAddress(sock_addr, local_addr);
    OnConnected(fd, local_addr);

    return true;
}

void TcpClient::OnError(const char* errstr)
{
    printf("[TcpClient::OnError] error string: %s\n", errstr);
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

}  // namespace richinfo
