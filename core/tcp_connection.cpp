#include "eventloop.h"
#include "tcp_connection.h"
#include <unistd.h>
#include <sstream>
#include "logger.h"

namespace evt_loop {

TcpConnection::TcpConnection(int fd, const IPAddress& local_addr, const IPAddress& peer_addr, const IPAddress& peer_real_addr,
    const OnClosedCallback& close_cb, TcpCallbacksPtr tcp_evt_cbs) :
  BufferIOEvent(IOType::TCP_CONNECTION, fd), id_(0), client_type_(0), local_addr_(local_addr), peer_addr_(peer_addr), peer_real_addr_(peer_real_addr),
  active_closing_(false), is_client_(false), creator_notification_cb_(close_cb), tcp_evt_cbs_(tcp_evt_cbs),
  heartbeat_handler_(this), checking_idle_timer_(nullptr)
{
    el_logger->info("[TcpConnection::TcpConnection] local_addr: {}, peer_addr: {}, peer_real_addr: {}",
        local_addr_.ToString(), peer_addr_.ToString(), peer_real_addr_.ToString());
}

TcpConnection::~TcpConnection()
{
    Destroy();
}

void TcpConnection::Destroy()
{
    el_logger->info("[TcpConnection::Destroy] id: {}, fd: {}", id_, fd_);
    if (state_ >= COUNT) return;    // Invalid connection

    DisableIdleTimeout();
    DisableHeartbeat();
    id_ = 0;
    checking_idle_timer_ = nullptr;
}

void TcpConnection::EnableHeartbeat(uint32_t idle_interval, uint32_t ping_interval, uint32_t ping_total)
{
    if (IsHeartbeatEnabled()) return;
    heartbeat_handler_.EnablePing(idle_interval, ping_interval, ping_total);
    el_logger->info("[TcpConnection::EnableHeartbeat] heartbeat enabled");
}

void TcpConnection::DisableHeartbeat()
{
    if (! IsHeartbeatEnabled()) return;
    heartbeat_handler_.DisablePing();
    el_logger->info("[TcpConnection::EnableHeartbeat] heartbeat disabled");
}

bool TcpConnection::IsHeartbeatEnabled() const
{
    return heartbeat_handler_.IsEnabled();
}

void TcpConnection::EnableIdleTimeout(uint32_t seconds, const OnIdleTimeoutCallback& cb)
{
    if (! tcp_evt_cbs_) return;
    tcp_evt_cbs_->on_idle_timeout_cb = cb;

    DisableIdleTimeout();

    TimeVal tv(seconds, 0);
    checking_idle_timer_ = std::make_shared<PeriodicTimer>(tv, std::bind(&TcpConnection::OnIdleTimeout, this, std::placeholders::_1));
    checking_idle_timer_->Start();
    el_logger->info("[TcpConnection::EnableIdleTimeout] idle timeout callback enabled");
}
void TcpConnection::DisableIdleTimeout()
{
    if (state_ >= COUNT) return;    // Invalid connection

    if (IsIdleTimeoutEnabled()) {
        el_logger->debug("[TcpConnection::DisableIdleTimeout] fd: {}", fd_);
        checking_idle_timer_->Stop();
        el_logger->info("[TcpConnection::EnableIdleTimeout] idle timeout callback disabled");
    }
}
bool TcpConnection::IsIdleTimeoutEnabled() const
{
    return checking_idle_timer_ && checking_idle_timer_->IsRunning();
}
void TcpConnection::OnIdleTimeout(TimerEvent* timer)
{
    el_logger->debug("[TcpConnection::OnIdleTimeout] fd: {} now: {}, stats_rx_last_time: {}, timer interval: {}",
            fd_, Now(), StatsRxLastTime(), timer->GetInterval().Seconds());
    if (Now() - StatsRxLastTime() > timer->GetInterval().Seconds()) {
        if (tcp_evt_cbs_) tcp_evt_cbs_->on_idle_timeout_cb(this, StatsRxLastTime());
    }
}

void TcpConnection::Disconnect()
{
    el_logger->warn("[TcpConnection::Disconnect] fd: {}, state: {}", fd_, state_);
    if (state_ == CLOSED || state_ >= COUNT) return;    // Invalid connection

    active_closing_ = true;
    if (TxBuffEmpty())
        OnClosed();
    else
        SetCloseWait();
}

void TcpConnection::OnReady()
{
    el_logger->info("[TcpConnection::OnReady]");
    if (on_conn_ready_cb_) on_conn_ready_cb_(this);
    if (tcp_evt_cbs_) tcp_evt_cbs_->on_conn_ready_cb(this);
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
            el_logger->warn("[TcpConnection::OnReceived] Invalid connection: {}", ToString());
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
    el_logger->info("[TcpConnection::OnClosed] fd: {}, state: {}, active_closing: {}", fd_, state_, active_closing_);
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
    el_logger->error("[TcpConnection::OnError] fd: {}, errcode: {}, errstr: {}", fd_, errcode, errstr);
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
