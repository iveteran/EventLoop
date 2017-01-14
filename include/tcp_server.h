#ifndef _TCP_SERVER_H
#define _TCP_SERVER_H

#include "tcp_connection.h"

namespace evt_loop {

class TcpServer: public IOEvent
{
    public:
    TcpServer(const char *host ="", uint16_t port=0, MessageType msg_type = MessageType::BINARY, TcpCallbacksPtr tcp_evt_cbs = nullptr);
    ~TcpServer();
    void Destroy();

    TcpConnectionPtr GetConnectionByFD(int fd);

    void SetTcpCallbacks(const TcpCallbacksPtr& tcp_evt_cbs);
    void SetNewClientCallback(const OnNewClientCallback& new_client_cb);
    void SetErrorCallback(const OnServerErrorCallback& error_cb);

    protected:
    virtual void InitAddress(const char* host, uint16_t port);
    virtual bool Start();
    virtual int AcceptClient(IPAddress& peer_addr);
    virtual TcpConnectionPtr CreateClient(int fd, const IPAddress& local_addr, const IPAddress& peer_addr)
    {
        return std::make_shared<TcpConnection>(fd, server_addr_, peer_addr,
                std::bind(&TcpServer::OnConnectionClosed, this, std::placeholders::_1), tcp_evt_cbs_);
    }

    void OnError(int errcode, const char* errstr);
    void OnEvents(uint32_t events);
    void OnNewClient(int fd, const IPAddress& peer_addr);
    void OnConnectionClosed(TcpConnection* conn);

    protected:
    IPAddress       server_addr_;
    MessageType     msg_type_;
    FdTcpConnMap    conn_map_;

    OnNewClientCallback     new_client_cb_;
    OnServerErrorCallback   error_cb_;
    TcpCallbacksPtr         tcp_evt_cbs_;
};

class TcpServer6: public TcpServer
{
    public:
    TcpServer6(const char *host="", uint16_t port=0, bool ipv6_only = true, MessageType msg_type = MessageType::BINARY, TcpCallbacksPtr tcp_evt_cbs = nullptr);

    protected:
    void InitAddress(const char* host, uint16_t port);
    bool Start();
    int AcceptClient(IPAddress& peer_addr);

    private:
    bool ipv6_only_;
};

}  // namespace evt_loop

#endif  // _TCP_SERVER_H
