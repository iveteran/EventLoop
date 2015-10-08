#ifndef _TCP_SERVER_H
#define _TCP_SERVER_H

#include <map>
#include "tcp_connection.h"

using std::map;

namespace evt_loop {

class TcpServer: public TcpCreator
{
    public:
    TcpServer(const char *host, uint16_t port, ITcpEventHandler* tcp_evt_handler = NULL);
    ~TcpServer();
    void SetTcpEventHandler(ITcpEventHandler* evt_handler);
    TcpConnection* GetConnectionByFD(int fd);

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

    ITcpEventHandler*    tcp_evt_handler_;
};

}  // namespace evt_loop

#endif  // _TCP_SERVER_H
