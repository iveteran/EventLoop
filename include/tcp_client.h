#ifndef _TCP_CLIENT_H
#define _TCP_CLIENT_H

#include <string>
#include <list>
#include "tcp_connection.h"
#include "timer_handler.h"

using std::string;
using std::list;

namespace evt_loop {

class TcpClient : public IOEvent
{
    public:
    TcpClient(const char *host="", uint16_t port=0, MessageType msg_type = MessageType::BINARY,
          bool auto_reconnect = true, TcpCallbacksPtr tcp_evt_cbs = nullptr);
    ~TcpClient();

    bool Connect();
    void Disconnect();

    void EnableKeepAlive(bool enable);
    void EnableHeartbeat(uint32_t idle_interval = TcpHeartbeatHandler::DFT_IDLE_INTERVAL,
            uint32_t ping_interval = TcpHeartbeatHandler::DFT_PING_INTERVAL,
            uint32_t ping_total = TcpHeartbeatHandler::DFT_PING_TOTAL);
    void EnableIdleTimeout(uint32_t seconds, const OnIdleTimeoutCallback& cb);

    void SetTcpCallbacks(const TcpCallbacksPtr& tcp_evt_cbs);
    void SetNewClientCallback(const OnNewClientCallback& new_client_cb);
    void SetErrorCallback(const OnClientErrorCallback& error_cb);

    TcpConnectionPtr& Connection() { return conn_; }
    int FD() const { return (conn_ ? conn_->FD() : -1); }  // Overrides interface of base class IOEvent
    bool IsConnected() const { return conn_ != nullptr; }

    bool Send(const string& msg);
    
    protected:
    void OnEvents(uint32_t events) {}
    void SetFD(int fd) { if (conn_) conn_->SetFD(fd); }  // Hides interface of base class IOEvent

    virtual void InitAddress(const char* host, uint16_t port);
    virtual bool Connect_();
    virtual TcpConnectionPtr CreateClient(int fd, const IPAddress& local_addr, const IPAddress& peer_addr, const IPAddress& peer_real_addr)
    {
        return std::make_shared<TcpConnection>(fd, local_addr, peer_addr, peer_real_addr,
                std::bind(&TcpClient::OnConnectionClosed, this, std::placeholders::_1), tcp_evt_cbs_);
    }
    void Reconnect();

    void OnConnected(int fd, const IPAddress& local_addr);
    void OnConnectionClosed(TcpConnection* conn);
    void OnError(int errcode, const char* errstr);
    void OnReconnectTimer(TimerEvent* timer);
    void OnReady(TcpConnection* conn) { SendTempBuffer(); }

    void SendTempBuffer();

  protected:
    IPAddress           server_addr_;
    MessageType         msg_type_;
    bool                keepalive_;
    bool                auto_reconnect_;
    TcpConnectionPtr    conn_;
    list<string>        tmp_sendbuf_list_;
    PeriodicTimer       reconnect_timer_;

    OnNewClientCallback     new_client_cb_;
    OnClientErrorCallback   error_cb_;
    TcpCallbacksPtr         tcp_evt_cbs_;

    HeartbeatParamsPtr      hb_tmp_params_;
    IdleTimeoutParamsPtr    idle_timeout_params_;
};
typedef std::shared_ptr<TcpClient> TcpClientPtr;

class TcpClient6 : public TcpClient
{
  public:
    TcpClient6(const char *host="", uint16_t port=0, MessageType msg_type = MessageType::BINARY,
          bool auto_reconnect = true, TcpCallbacksPtr tcp_evt_cbs = nullptr);

  protected:
    virtual void InitAddress(const char* host, uint16_t port);
    virtual bool Connect_();
};
typedef std::shared_ptr<TcpClient6> TcpClient6Ptr;

}  // namespace evt_loop

#endif  // _TCP_CLIENT_H
