#include "tcp_connection.h"

namespace evt_loop {

void SocketAddrToIPAddress(const struct sockaddr_in& sock_addr, IPAddress& ip_addr)
{
  char buffer[INET_ADDRSTRLEN] = {0};
  inet_ntop(sock_addr.sin_family, (void*)&sock_addr.sin_addr, buffer, sizeof(buffer));
  ip_addr.ip_.assign(buffer);
  ip_addr.port_ = sock_addr.sin_port;
}

TcpConnection::TcpConnection(int fd, const IPAddress& local_addr, const IPAddress& peer_addr, TcpCallbacksPtr tcp_evt_cbs, TcpCreator* creator)
    : id_(0), ready_(false), local_addr_(local_addr), peer_addr_(peer_addr), tcp_evt_cbs_(tcp_evt_cbs), creator_(creator)
{
    SetReady(fd);
    printf("[TcpConnection::TcpConnection] local_addr: %s, peer_addr: %s\n", local_addr_.ToString().c_str(), peer_addr_.ToString().c_str());
}

TcpConnection::~TcpConnection()
{
    Disconnect();
}

void TcpConnection::SetTcpCallbacks(const TcpCallbacksPtr& tcp_evt_cbs)
{
    tcp_evt_cbs_ = tcp_evt_cbs;
}

void TcpConnection::Disconnect()
{
    EV_Singleton->DeleteEvent(this);
    if (fd_ >= 0) close(fd_);
    SetFD(-1);
    ready_ = false;
}

void TcpConnection::SetReady(int fd)
{
    if (fd < 0) return;

    SetFD(fd);
    EV_Singleton->AddEvent(this);
    ready_ = true;
    if (!TxBuffEmpty()) {
        AddWriteEvent();
    }
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

void TcpConnection::OnReceived(const Message* msg)
{
    if (tcp_evt_cbs_) tcp_evt_cbs_->on_msg_recvd_cb(this, msg);
}

void TcpConnection::OnSent(const Message* msg)
{
    if (tcp_evt_cbs_) tcp_evt_cbs_->on_msg_sent_cb(this, msg);
}

void TcpConnection::OnClosed()
{
    printf("[TcpConnection::OnClosed] client leave, fd: %d\n", fd_);
    if (tcp_evt_cbs_) tcp_evt_cbs_->on_closed_cb(this);
    if (creator_) {
        creator_->OnConnectionClosed(this);
    } else {
        Disconnect();
    }
}

void TcpConnection::OnError(int errcode, const char* errstr)
{
    printf("[TcpConnection::OnError] error string: %s\n", errstr);
    if (tcp_evt_cbs_) tcp_evt_cbs_->on_error_cb(errcode, errstr);
    //OnClosed();
}

}  // namespace evt_loop
