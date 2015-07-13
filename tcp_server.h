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
    void SetOnMessageCb(const OnMessageCallback& on_msg_cb);

    private:
    bool Listen();
    void OnEvents(uint32_t events);
    void OnNewClient(int fd, const IPAddress& peer_addr);

    void OnConnectionClosed(TcpConnection* conn);
    void OnError(const char* errstr);

    private:
    IPAddress server_addr_;
    map<int/*fd*/, TcpConnection*> conn_map_;
    OnMessageCallback on_msg_cb_;
};

}  // namespace richinfo

#endif  // _TCP_SERVER_H
