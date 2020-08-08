#ifndef _TLS_CLIENT_H
#define _TLS_CLIENT_H

#include "tcp_server.h"
#include "tls_connection.h"

namespace evt_loop {

class TLSClient : public TcpClient
{
    public:
    TLSClient(const char *host ="", uint16_t port=0, MessageType msg_type = MessageType::BINARY, bool auto_reconnect = true,
            TcpCallbacksPtr tcp_evt_cbs = nullptr) : TcpClient(host, port, msg_type, auto_reconnect, tcp_evt_cbs)
    { }

    protected:
    TcpConnectionPtr CreateClient(int fd, const IPAddress& local_addr, const IPAddress& peer_addr, const IPAddress& peer_real_addr)
    {
        return std::make_shared<TLSConnection>(fd, server_addr_, peer_addr, peer_real_addr,
                std::bind(&TLSClient::OnConnectionClosed, this, std::placeholders::_1), tcp_evt_cbs_);
    }

    void OnConnectionClosed(TcpConnection* conn)
    {
        TcpClient::OnConnectionClosed(conn);
    }
};
typedef std::shared_ptr<TLSClient> TLSClientPtr;

}  // ns evt_loop

#endif  // _TLS_CLIENT_H
