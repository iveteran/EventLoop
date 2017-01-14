#ifndef _TLS_SERVER_H
#define _TLS_SERVER_H

#include "tcp_server.h"
#include "tls_connection.h"

namespace evt_loop {

class TLSServer : public TcpServer
{
    public:
    TLSServer(const char *host ="", uint16_t port=0, MessageType msg_type = MessageType::BINARY, TcpCallbacksPtr tcp_evt_cbs = nullptr) : TcpServer(host, port, msg_type, tcp_evt_cbs)
    { }

    protected:
    TcpConnectionPtr CreateClient(int fd, const IPAddress& local_addr, const IPAddress& peer_addr)
    {
        return std::make_shared<TLSConnection>(fd, server_addr_, peer_addr,
                std::bind(&TLSServer::OnConnectionClosed, this, std::placeholders::_1), tcp_evt_cbs_);
    }

    void OnConnectionClosed(TcpConnection* conn)
    {
        TcpServer::OnConnectionClosed(conn);
    }
};

}  // ns evt_loop

#endif  // _TLS_SERVER_H
