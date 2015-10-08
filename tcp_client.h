#ifndef _TCP_CLIENT_H
#define _TCP_CLIENT_H

#include "tcp_connection.h"

namespace richinfo {

class TcpClient : public TcpCreator
{
    public:
    TcpClient(const char *host, uint16_t port, bool auto_reconnect = true, ITcpEventHandler* tcp_evt_handler = NULL);
    ~TcpClient();
    bool Connect();
    void Disconnect();
    bool Send(const string& msg);
    void SetTcpEventHandler(ITcpEventHandler* evt_handler);
    TcpConnection* Connection() { return conn_; }
    int FD() const { return (conn_ ? conn_->FD() : -1); }  // Overrides interface of base class IOEvent
    
    private:
    void OnEvents(uint32_t events) {}
    void SetFD(int fd);  // Hides interface of base class IOEvent

    bool Connect_();
    void Reconnect();

    void OnConnected(int fd, const IPAddress& local_addr);
    void OnConnectionClosed(TcpConnection* conn);
    void OnError(int errcode, const char* errstr);

    void SendTempBuffer();

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
    bool            auto_reconnect_;
    IPAddress       server_addr_;
    TcpConnection*  conn_;
    list<string>    tmp_sendbuf_list_;
    ReconnectTimer  reconnect_timer_;

    ITcpEventHandler*    tcp_evt_handler_;
};

}  // namespace richinfo

#endif  // _TCP_CLIENT_H
