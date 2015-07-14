#ifndef _TCP_SERVER_H
#define _TCP_SERVER_H

#include <map>
#include "tcp_connection.h"

using std::map;

namespace richinfo {

class TcpServer: public TcpCreator
{
    public:
    TcpServer(const char *host, uint16_t port);
    ~TcpServer();
    void SetOnMsgRecvdCb(const OnMsgRecvdCallback& cb);

    protected:
    void OnError(int errcode, const char* errstr);

    private:
    bool Start();
    void Destory();

    void OnEvents(uint32_t events);
    void OnNewClient(int fd, const IPAddress& peer_addr);
    void OnConnectionClosed(TcpConnection* conn);

    private:
    IPAddress server_addr_;
    map<int/*fd*/, TcpConnection*> conn_map_;

    TcpConnEventCallbacks   conn_callbacks_;
    TcpEventCallbacks       svr_callbacks_;
};

}  // namespace richinfo

#endif  // _TCP_SERVER_H
