#ifndef _TCP_CONNECTION_H
#define _TCP_CONNECTION_H

#include <map>
#include <memory>

#include "fd_handler.h"
#include "tcp_callbacks.h"
#include "tcp_heartbeat_handler.h"
#include "utils.h"

using std::map;
using std::shared_ptr;

namespace evt_loop {

class TcpConnection : public BufferIOEvent
{
  public:
    TcpConnection(int fd, const IPAddress& local_addr, const IPAddress& peer_addr, const IPAddress& peer_real_addr,
            const OnClosedCallback& close_cb, TcpCallbacksPtr tcp_evt_cbs = nullptr);
    ~TcpConnection();

    uint32_t ID() const { return id_; }
    void SetID(uint32_t id) { id_ = id; }

    uint8_t ClientType() const { return client_type_; }
    void SetClientType(uint8_t ctype) { client_type_ = ctype; }

    void AsClient() { is_client_ = true; }
    bool IsClient() const { return is_client_; }
    void Disconnect();

    void SetTcpCallbacks(const TcpCallbacksPtr& tcp_evt_cbs);
    void SetReadyCallback(const OnReadyCallback& cb);

    void EnableHeartbeat(uint32_t idle_interval = TcpHeartbeatHandler::DFT_IDLE_INTERVAL,
            uint32_t ping_interval = TcpHeartbeatHandler::DFT_PING_INTERVAL,
            uint32_t ping_total = TcpHeartbeatHandler::DFT_PING_TOTAL);
    void DisableHeartbeat();

    const IPAddress& GetLocalAddr() const;
    const IPAddress& GetPeerAddr() const;
    const IPAddress& GetPeerRealAddr() const;
    void SetPeerRealAddr(const IPAddress& addr) { peer_real_addr_ = addr; }

  protected:
    void Destroy();
    void OnReceived(const Message* buffer);
    void OnSent(const Message* buffer);
    void OnClosed();
    void OnError(int errcode, const char* errstr);
    void OnReady()
    {
        printf("TcpConnection::OnReady\n");
        if (on_conn_ready_cb_) on_conn_ready_cb_(this);
        if (tcp_evt_cbs_) tcp_evt_cbs_->on_conn_ready_cb(this);
    }

  private:
    uint32_t        id_;
    uint8_t         client_type_;
    IPAddress       local_addr_;
    IPAddress       peer_addr_;
    IPAddress       peer_real_addr_;
    bool            active_closing_;
    bool            is_client_;

    OnReadyCallback   on_conn_ready_cb_;
    OnClosedCallback  creator_notification_cb_;
    TcpCallbacksPtr   tcp_evt_cbs_;

    TcpHeartbeatHandler heartbeat_handler_;
};

typedef shared_ptr<TcpConnection>          TcpConnectionPtr;
typedef map<int/*fd*/, TcpConnectionPtr>   FdTcpConnMap;

}  // namespace evt_loop

#endif  // _TCP_CONNECTION_H
