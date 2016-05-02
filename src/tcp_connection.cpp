#include "eventloop.h"
#include "tcp_connection.h"
#include <unistd.h>

namespace evt_loop {

TcpConnection::TcpConnection(int fd, const IPAddress& local_addr, const IPAddress& peer_addr,
    const OnClosedCallback& close_cb, TcpCallbacksPtr tcp_evt_cbs) :
  BufferIOEvent(fd), id_(0), local_addr_(local_addr), peer_addr_(peer_addr),
  creator_notification_cb_(close_cb), tcp_evt_cbs_(tcp_evt_cbs)
{
    printf("[TcpConnection::TcpConnection] local_addr: %s, peer_addr: %s\n",
        local_addr_.ToString().c_str(), peer_addr_.ToString().c_str());
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
    if (fd_ >= -1) {
        close(fd_);
        SetFD(-1);
    }
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
    creator_notification_cb_(this);
}

void TcpConnection::OnError(int errcode, const char* errstr)
{
    printf("[TcpConnection::OnError] error string: %s\n", errstr);
    if (tcp_evt_cbs_) tcp_evt_cbs_->on_error_cb(errcode, errstr);
    //OnClosed();
}

}  // namespace evt_loop
