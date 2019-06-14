#include "eventloop.h"
#include "tcp_connection.h"
#include <unistd.h>
#include <sstream>

namespace evt_loop {

TcpConnection::TcpConnection(int fd, const IPAddress& local_addr, const IPAddress& peer_addr, const IPAddress& peer_real_addr,
    const OnClosedCallback& close_cb, TcpCallbacksPtr tcp_evt_cbs) :
  BufferIOEvent(fd), id_(0), client_type_(0), local_addr_(local_addr), peer_addr_(peer_addr), peer_real_addr_(peer_real_addr),
  active_closing_(false), is_client_(false), creator_notification_cb_(close_cb), tcp_evt_cbs_(tcp_evt_cbs),
  heartbeat_handler_(this), checking_idle_timer_(nullptr)
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
    if (state_ >= COUNT) return;    // Invalid connection

    DisableIdleTimeout();
    DisableHeartbeat();
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

void TcpConnection::EnableIdleTimeout(uint32_t seconds, const OnIdleTimeoutCallback& cb)
{
    if (! tcp_evt_cbs_) return;
    tcp_evt_cbs_->on_idle_timeout_cb = cb;

    DisableIdleTimeout();

    TimeVal tv(seconds, 0);
    checking_idle_timer_ = std::make_shared<PeriodicTimer>(tv, std::bind(&TcpConnection::OnIdleTimeout, this, std::placeholders::_1));
    checking_idle_timer_->Start();
}
void TcpConnection::DisableIdleTimeout()
{
    if (state_ >= COUNT) return;    // Invalid connection

    if (checking_idle_timer_ && checking_idle_timer_->IsRunning()) {
        printf("[TcpConnection::DisableIdleTimeout] fd: %d\n", fd_);
        checking_idle_timer_->Stop();
    }
}
void TcpConnection::OnIdleTimeout(TimerEvent* timer)
{
    printf("[TcpConnection::OnIdleTimeout] fd: %d now: %ld, stats_rx_last_time: %ld, timer interval: %d\n",
            fd_, Now(), StatsRxLastTime(), timer->GetInterval().Seconds());
    if (Now() - StatsRxLastTime() > timer->GetInterval().Seconds()) {
        if (tcp_evt_cbs_) tcp_evt_cbs_->on_idle_timeout_cb(this, StatsRxLastTime());
    }
}

void TcpConnection::Disconnect()
{
    printf("[TcpConnection::Disconnect] fd: %d, state: %d\n", fd_, state_);
    if (state_ == CLOSED || state_ >= COUNT) return;    // Invalid connection

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
        if (state_ >= CLOSED && state_ < COUNT)     // Guard condition
        {
            tcp_evt_cbs_->on_msg_recvd_cb(this, msg);
        }
        else
        {
            printf("[TcpConnection::OnReceived] Invalid connection: %s\n", ToString().c_str());
        }
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
    printf("[TcpConnection::OnClosed] fd: %d, state: %d, active_closing: %d\n", fd_, state_, active_closing_);
    if (state_ == CLOSED || state_ >= COUNT) return;    // Invalid connection

    if (active_closing_) {
        creator_notification_cb_(this);  // NOTE: acitve closing mode, MUST run this line before Destroy()
        Destroy();
    } else {
        if (tcp_evt_cbs_) tcp_evt_cbs_->on_closed_cb(this);
        creator_notification_cb_(this);  // NOTE: passivity closing mode, MUST run this line after close callback
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

string TcpConnection::ToString() const
{
    if (state_ >= COUNT) return "{ Invalid Connection }";

    std::stringstream ss;
    ss << "{ "
        << "fd: " << fd_ << ", "
        << "id: " << id_ << ", "
        << "client_type: " << int(client_type_) << ", "
        << "local_addr: " << local_addr_.ToString() << ", "
        << "peer_addr: " << peer_addr_.ToString() << ", "
        << "peer_real_addr: " << peer_real_addr_.ToString() << ", "
        << "active_closing: " << active_closing_ << ", "
        << "is_client: " << is_client_
        << " }";
    return ss.str();
}

}  // namespace evt_loop
