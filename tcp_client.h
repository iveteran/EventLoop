#ifndef _TCP_CLIENT_H
#define _TCP_CLIENT_H

#include "tcp_connection.h"

namespace richinfo {

class TcpClient : public TcpCreator
{
    public:
    TcpClient(const char *host, uint16_t port, bool auto_reconnect = true);
    bool Connect();
    bool Send(const string& msg);
    void SetOnMessageCb(const OnMessageCallback& on_msg_cb);
    TcpConnection* Connection() { return conn_; }
    
    private:
    void OnEvents(uint32_t events) {}

    bool Connect_();
    void Reconnect() { reconnect_timer_.Start(); }

    void OnConnected(int fd, const IPAddress& local_addr);
    void OnConnectionClosed(TcpConnection* conn);
    void OnError(const char* errstr);

    private:
    class ReconnectTimer : public PeriodicTimerEvent
    {
        friend class TcpClient;
        public:
        ReconnectTimer(TcpClient* creator) : creator_(creator) { }
        void OnTimer();

        private:
        TcpClient* creator_;
    };

  private:
    bool auto_reconnect_;
    IPAddress server_addr_;
    TcpConnection* conn_;
    ReconnectTimer reconnect_timer_;
    OnMessageCallback on_msg_cb_;
};

}  // namespace richinfo

#endif  // _TCP_CLIENT_H
