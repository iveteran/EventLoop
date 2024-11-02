#ifndef _TCP_SERVER_H
#define _TCP_SERVER_H

#include "tcp_connection.h"
#include "ip_addr.h"

namespace evt_loop {

class TcpServer: public IOEvent
{
    public:
    TcpServer(IPVer ip_ver, const char *host ="", uint16_t port=0,
            MessageType msg_type = MessageType::BINARY,
            TcpCallbacksPtr tcp_evt_cbs = nullptr);
    ~TcpServer();
    void Destroy();
    IPVer GetIPVersion() const { return ip_ver_; }

    bool Start();
    const IPAddress& GetAddress() const { return server_addr_; }
    TcpConnectionPtr GetConnectionByFD(int fd);
    uint32_t GetConnectionNumber() const { return conn_map_.size(); }

    void SetMessageHeaderDescription(const HeaderDescriptionPtr& msg_hdr_desc) {
        msg_hdr_desc_ = msg_hdr_desc;
    }
    const HeaderDescriptionPtr& GetMessageHeaderDescription() const {
      return msg_hdr_desc_;
    }

    void SetTcpCallbacks(const TcpCallbacksPtr& tcp_evt_cbs);
    void SetNewClientCallback(const OnNewClientCallback& new_client_cb) { new_client_cb_ = new_client_cb; }
    void SetErrorCallback(const OnServerErrorCallback& error_cb) { error_cb_ = error_cb; }
    void EnableHeartbeat(uint32_t idle_interval = TcpHeartbeatHandler::DFT_IDLE_INTERVAL,
            uint32_t ping_interval = TcpHeartbeatHandler::DFT_PING_INTERVAL,
            uint32_t ping_total = TcpHeartbeatHandler::DFT_PING_TOTAL);
    void EnableIdleTimeout(uint32_t seconds, const OnIdleTimeoutCallback& cb);

    protected:
    void InitAddress(const char* host, uint16_t port);
    void InitV6Address(const char* host, uint16_t port);
    int AcceptClient(IPAddress& peer_addr);
    int AcceptClient6(IPAddress& peer_addr);
    TcpConnectionPtr CreateClient(int fd, const IPAddress& local_addr,
            const IPAddress& peer_addr, const IPAddress& peer_real_addr);

    void OnError(int errcode, const char* errstr);
    void OnEvents(uint32_t events, void* ctx = nullptr) override;
    void OnNewClient(int fd, const IPAddress& peer_addr);
    void OnConnectionClosed(TcpConnection* conn);

    protected:
    IPVer           ip_ver_;
    IPAddress       server_addr_;

    MessageType     msg_type_;
    FdTcpConnMap    conn_map_;

    HeaderDescriptionPtr    msg_hdr_desc_;

    OnNewClientCallback     new_client_cb_;
    OnServerErrorCallback   error_cb_;
    TcpCallbacksPtr         tcp_evt_cbs_;
    HeartbeatParamsPtr      hb_tmp_params_;
    IdleTimeoutParamsPtr    idle_timeout_params_;
};
typedef std::shared_ptr<TcpServer> TcpServerPtr;

}  // namespace evt_loop

#endif  // _TCP_SERVER_H
