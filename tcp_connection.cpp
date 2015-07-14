#include "eventloop.h"
#include "tcp_connection.h"

namespace richinfo {

void SocketAddrToIPAddress(const struct sockaddr_in& sock_addr, IPAddress& ip_addr)
{
  char buffer[INET_ADDRSTRLEN] = {0};
  inet_ntop(sock_addr.sin_family, (void*)&sock_addr.sin_addr, buffer, sizeof(buffer));
  ip_addr.ip_.assign(buffer);
  ip_addr.port_ = sock_addr.sin_port;
}

TcpConnection::TcpConnection(int fd, const IPAddress& local_addr, const IPAddress& peer_addr, TcpCreator* creator)
    : ready_(false), local_addr_(local_addr), peer_addr_(peer_addr), creator_(creator)
{
    SetReady(fd);
    printf("[TcpConnection::TcpConnection] local_addr: %s, peer_addr: %s\n", local_addr_.ToString().c_str(), peer_addr_.ToString().c_str());
}

TcpConnection::~TcpConnection()
{
    Disconnect();
}

void TcpConnection::SetCallbacks(const TcpConnEventCallbacks& cbs)
{
    callbacks_ = cbs;
}
void TcpConnection::SetOnMsgRecvdCb(const OnMsgRecvdCallback& cb)
{
    callbacks_.on_msg_recvd_cb_ = cb;
}
void TcpConnection::SetOnMsgSentCb(const OnMsgSentCallback& cb)
{
    callbacks_.on_msg_sent_cb_ = cb;
}
void TcpConnection::SetOnClosedCb(const OnClosedCallback& cb)
{
    callbacks_.on_closed_cb_ = cb;
}
void TcpConnection::SetOnErrorCb(const OnErrorCallback& cb)
{
    callbacks_.on_error_cb_ = cb;
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
    //printf("[TcpConnection::OnReceived] received string: %s\n", buffer.c_str());
    //on_msg_cb_.Invoke(std::make_tuple(this, &buffer));
    callbacks_.on_msg_recvd_cb_.Invoke(this, &buffer);
}

void TcpConnection::OnSent(const string& buffer)
{
    callbacks_.on_msg_sent_cb_.Invoke(this, &buffer);
}

void TcpConnection::OnClosed()
{
    printf("[TcpConnection::OnClosed] client leave, fd: %d\n", fd_);
    if (creator_) {
        creator_->OnConnectionClosed(this);
    } else {
        Disconnect();
    }
    callbacks_.on_closed_cb_.Invoke(this);
}

void TcpConnection::OnError(int errcode, const char* errstr)
{
    printf("[TcpConnection::OnError] error string: %s\n", errstr);
    callbacks_.on_error_cb_.Invoke(errcode, errstr);
    //OnClosed();
}

}  // namespace richinfo
