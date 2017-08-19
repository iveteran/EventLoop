#include "eventloop.h"
#include "tcp_connection.h"
#include <unistd.h>

namespace evt_loop {

TcpConnection::TcpConnection(int fd, const IPAddress& local_addr, const IPAddress& peer_addr, const IPAddress& peer_real_addr,
    const OnClosedCallback& close_cb, TcpCallbacksPtr tcp_evt_cbs) :
  BufferIOEvent(fd), id_(0), client_type_(0), local_addr_(local_addr), peer_addr_(peer_addr), peer_real_addr_(peer_real_addr),
  active_closing_(false), is_client_(false), creator_notification_cb_(close_cb), tcp_evt_cbs_(tcp_evt_cbs)
{
    printf("[TcpConnection::TcpConnection] local_addr: %s, peer_addr: %s, peer_real_addr: %s\n",
        local_addr_.ToString().c_str(), peer_addr_.ToString().c_str(), peer_real_addr_.ToString().c_str());
}

TcpConnection::~TcpConnection()
{
    Destroy();
}

void TcpConnection::Destroy()
{
    printf("[TcpConnection::Destroy] id: %d, fd: %d\n", id_, fd_);
    if (fd_ >= 0) {
        close(fd_);
        SetFD(-1);
    }
}

void TcpConnection::SetTcpCallbacks(const TcpCallbacksPtr& tcp_evt_cbs)
{
    tcp_evt_cbs_ = tcp_evt_cbs;
}

void TcpConnection::SetReadyCallback(const OnReadyCallback& cb)
{
    on_conn_ready_cb_ = cb;
}

void TcpConnection::Disconnect()
{
    active_closing_ = true;
    if (TxBuffEmpty())
        OnClosed();
    else
        SetCloseWait();
}

const IPAddress& TcpConnection::GetLocalAddr() const
{
    return local_addr_;
}

const IPAddress& TcpConnection::GetPeerAddr() const
{
    return peer_addr_;
}

const IPAddress& TcpConnection::GetPeerRealAddr() const
{
    return peer_real_addr_;
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
    if (active_closing_) {
        creator_notification_cb_(this);  // NOTE: MUST run this line before Destroy()
        Destroy();
    } else {
        if (tcp_evt_cbs_) tcp_evt_cbs_->on_closed_cb(this);
        creator_notification_cb_(this);  // NOTE: MUST run this line after close callback
    }
    state_ = CLOSED;
}

void TcpConnection::OnError(int errcode, const char* errstr)
{
    printf("[TcpConnection::OnError] fd: %d, errcode: %d, errstr: %s\n", fd_, errcode, errstr);
    if (tcp_evt_cbs_) tcp_evt_cbs_->on_error_cb(this, errcode, errstr);
    //Disconnect();
    state_ = FAILED;
}

}  // namespace evt_loop
