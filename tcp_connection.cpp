#include "tcp_connection.h"

namespace richinfo {

void SocketAddrToIPAddress(const struct sockaddr_in& sock_addr, IPAddress& ip_addr)
{
  char buffer[INET_ADDRSTRLEN] = {0};
  inet_ntop(sock_addr.sin_family, (void*)&sock_addr.sin_addr, buffer, sizeof(buffer));
  ip_addr.ip_.assign(buffer);
  ip_addr.port_ = sock_addr.sin_port;
}

TcpConnection::TcpConnection(int fd, const IPAddress& local_addr, const IPAddress& peer_addr, ITcpEventHandler* tcp_evt_handler, TcpCreator* creator)
    : ready_(false), local_addr_(local_addr), peer_addr_(peer_addr), tcp_evt_handler_(tcp_evt_handler), creator_(creator)
{
    SetReady(fd);
    printf("[TcpConnection::TcpConnection] local_addr: %s, peer_addr: %s\n", local_addr_.ToString().c_str(), peer_addr_.ToString().c_str());
}

TcpConnection::~TcpConnection()
{
    Disconnect();
}

void TcpConnection::SetTcpEventHandler(ITcpEventHandler* evt_handler)
{
    tcp_evt_handler_ = evt_handler;
}

void TcpConnection::Disconnect()
{
    EV_Singleton->DeleteEvent(this);
    close(fd_);
    ready_ = false;
}

void TcpConnection::SetReady(int fd)
{
    if (fd < 0) return;

    SetFD(fd);
    EV_Singleton->AddEvent(this);
    ready_ = true;
}

bool TcpConnection::IsReady() const
{
    return ready_;
}

const IPAddress& TcpConnection::GetLocalAddr() const
{
    return local_addr_;
}

const IPAddress& TcpConnection::GetPeerAddr() const
{
    return peer_addr_;
}

void TcpConnection::OnReceived(const string& buffer)
{
    if (tcp_evt_handler_) tcp_evt_handler_->OnMessageRecvd(this, &buffer);
}

void TcpConnection::OnSent(const string& buffer)
{
    if (tcp_evt_handler_) tcp_evt_handler_->OnMessageSent(this, &buffer);
}

void TcpConnection::OnClosed()
{
    printf("[TcpConnection::OnClosed] client leave, fd: %d\n", fd_);
    if (tcp_evt_handler_) tcp_evt_handler_->OnConnectionClosed(this);
    if (creator_) {
        creator_->OnConnectionClosed(this);
    } else {
        Disconnect();
    }
}

void TcpConnection::OnError(int errcode, const char* errstr)
{
    printf("[TcpConnection::OnError] error string: %s\n", errstr);
    if (tcp_evt_handler_) tcp_evt_handler_->OnError(errcode, errstr);
    //OnClosed();
}

}  // namespace richinfo
