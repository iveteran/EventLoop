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
    TcpClient(IPVer ip_ver, const char *host="", uint16_t port=0,
            MessageType msg_type = MessageType::BINARY,
            bool auto_reconnect = true, TcpCallbacksPtr tcp_evt_cbs = nullptr);
    ~TcpClient();
    IPVer GetIPVersion() const { return ip_ver_; }

    bool Connect();
    void Disconnect();

    void EnableKeepAlive(bool enable);
    bool IsKeepAliveEnabled() const { return keepalive_; }
    void EnableHeartbeat(uint32_t idle_interval = TcpHeartbeatHandler::DFT_IDLE_INTERVAL,
            uint32_t ping_interval = TcpHeartbeatHandler::DFT_PING_INTERVAL,
            uint32_t ping_total = TcpHeartbeatHandler::DFT_PING_TOTAL);
    void DisableHeartbeat();
    bool IsHeartbeatEnabled() const;
    void EnableIdleTimeout(uint32_t seconds, const OnIdleTimeoutCallback& cb);
    void SetAutoReconnect(bool value = true) { auto_reconnect_ = value; }
    bool IsAutoReconnectEnabled() const { return auto_reconnect_; }

    void SetMessageHeaderDescription(const HeaderDescriptionPtr& msg_hdr_desc) {
        msg_hdr_desc_ = msg_hdr_desc;
    }
    const HeaderDescriptionPtr& GetMessageHeaderDescription() const {
      return msg_hdr_desc_;
    }

    void SetTcpCallbacks(const TcpCallbacksPtr& tcp_evt_cbs);
    void SetNewClientCallback(const OnNewClientCallback& new_client_cb);
    void SetErrorCallback(const OnClientErrorCallback& error_cb);

    TcpConnectionPtr& Connection() { return conn_; }
    int FD() const { return (conn_ ? conn_->FD() : -1); }  // Overrides interface of base class IOEvent
    bool IsConnected() const { return conn_ != nullptr; }

    bool Send(const string& msg);
    bool Send(const char* msg, size_t size);
    
    protected:
    void OnEvents(uint32_t events, void* ctx = nullptr) {}
    void SetFD(int fd) { if (conn_) conn_->SetFD(fd); }  // Hides interface of base class IOEvent

    void InitAddress(const char* host, uint16_t port);
    void InitAddress6(const char* host, uint16_t port);
    bool Connect_();
    TcpConnectionPtr CreateClient(int fd, const IPAddress& local_addr,
            const IPAddress& peer_addr, const IPAddress& peer_real_addr);
    void Reconnect();

    void OnConnected(int fd, const IPAddress& local_addr);
    void OnConnectionClosed(TcpConnection* conn);
    void OnError(int errcode, const char* errstr);
    void OnReconnectTimer(TimerEvent* timer);
    void OnReady(TcpConnection* conn) { SendTempBuffer(); }

    void SendTempBuffer();

  protected:
    IPVer               ip_ver_;
    IPAddress           server_addr_;
    MessageType         msg_type_;
    bool                keepalive_;
    bool                auto_reconnect_;
    TcpConnectionPtr    conn_;
    list<string>        tmp_sendbuf_list_;
    PeriodicTimer       reconnect_timer_;

    HeaderDescriptionPtr    msg_hdr_desc_;

    OnNewClientCallback     new_client_cb_;
    OnClientErrorCallback   error_cb_;
    TcpCallbacksPtr         tcp_evt_cbs_;

    HeartbeatParamsPtr      hb_tmp_params_;
    IdleTimeoutParamsPtr    idle_timeout_params_;
};
typedef std::shared_ptr<TcpClient> TcpClientPtr;

}  // namespace evt_loop

#endif  // _TCP_CLIENT_H
