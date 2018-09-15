#include "eventloop.h"
#include "tcp_connection.h"
#include <unistd.h>

namespace evt_loop {

TcpConnection::TcpConnection(int fd, const IPAddress& local_addr, const IPAddress& peer_addr, const IPAddress& peer_real_addr,
    const OnClosedCallback& close_cb, TcpCallbacksPtr tcp_evt_cbs) :
  BufferIOEvent(fd), id_(0), client_type_(0), local_addr_(local_addr), peer_addr_(peer_addr), peer_real_addr_(peer_real_addr),
  active_closing_(false), is_client_(false), creator_notification_cb_(close_cb), tcp_evt_cbs_(tcp_evt_cbs),
  heartbeat_handler_(this)
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

void TcpConnection::EnableHeartbeat(uint32_t idle_interval, uint32_t ping_interval, uint32_t ping_total)
{
    heartbeat_handler_.EnablePing(idle_interval, ping_interval, ping_total);
}

void TcpConnection::DisableHeartbeat()
{
    heartbeat_handler_.DisablePing();
}

void TcpConnection::Disconnect()
{
    if (state_ == CLOSED) return;

    active_closing_ = true;
    if (TxBuffEmpty())
        OnClosed();
    else
        SetCloseWait();
}

void TcpConnection::OnReceived(const Message* msg)
{
    if (heartbeat_handler_.IsHeartbeatRequest(msg)) {
        heartbeat_handler_.OnHeartbeatRequestReceived(msg);
    } else if (heartbeat_handler_.IsHeartbeatResponse(msg)) {
        heartbeat_handler_.OnHeartbeatResponseReceived(msg);
    } else if (tcp_evt_cbs_) {
        tcp_evt_cbs_->on_msg_recvd_cb(this, msg);
    }
}

void TcpConnection::OnSent(const Message* msg)
{
    if (heartbeat_handler_.IsHeartbeatRequest(msg)) {
        heartbeat_handler_.OnHeartbeatRequestSent(msg);
    } else if (heartbeat_handler_.IsHeartbeatResponse(msg)) {
        heartbeat_handler_.OnHeartbeatResponseSent(msg);
    } else if (tcp_evt_cbs_) {
        tcp_evt_cbs_->on_msg_sent_cb(this, msg);
    }
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
