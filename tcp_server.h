#ifndef _TCP_SERVER_H
#define _TCP_SERVER_H

#include <map>
#include <memory>
#include "tcp_connection.h"

namespace evt_loop {

typedef std::map<int/*fd*/, TcpConnection*>     FdTcpConnMap;
typedef std::shared_ptr<TcpCallbacks>           TcpCbsPtr;

class TcpServer: public TcpCreator
{
    public:
    TcpServer(const char *host, uint16_t port, TcpCallbacks* tcp_evt_cbs = NULL);
    ~TcpServer();
    void SetTcpCallbacks(TcpCallbacks* tcp_evt_cbs);
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
    IPAddress       server_addr_;
    FdTcpConnMap    conn_map_;
    TcpCallbacks*   tcp_evt_cbs_;
};

}  // namespace evt_loop

#endif  // _TCP_SERVER_H
