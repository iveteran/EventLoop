#ifndef _TCP_SERVER_H
#define _TCP_SERVER_H

#include "tcp_connection.h"

namespace evt_loop {

class TcpServer: public IOEvent
{
    public:
    TcpServer(const char *host, uint16_t port, MessageType msg_type = MessageType::BINARY, TcpCallbacksPtr tcp_evt_cbs = nullptr);
    ~TcpServer();

    TcpConnectionPtr GetConnectionByFD(int fd);

    void SetTcpCallbacks(const TcpCallbacksPtr& tcp_evt_cbs);
    void SetNewClientCallback(const OnNewClientCallback& new_client_cb);
    void SetErrorCallback(const OnServerErrorCallback& error_cb);

    protected:
    void OnError(int errcode, const char* errstr);

    private:
    bool Start();
    void Destory();

    void OnEvents(uint32_t events);
    void OnNewClient(int fd, const IPAddress& peer_addr);
    void OnConnectionClosed(TcpConnection* conn);

    private:
    IPAddress       server_addr_;
    MessageType     msg_type_;
    FdTcpConnMap    conn_map_;

    OnNewClientCallback     new_client_cb_;
    OnServerErrorCallback   error_cb_;
    TcpCallbacksPtr         tcp_evt_cbs_;
};

}  // namespace evt_loop

#endif  // _TCP_SERVER_H
