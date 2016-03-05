#ifndef _TCP_CLIENT_H
#define _TCP_CLIENT_H

#include "tcp_connection.h"

namespace evt_loop {

class TcpClient : public TcpCreator
{
    public:
    TcpClient(const char *host, uint16_t port, bool auto_reconnect = true, TcpCallbacksPtr tcp_evt_cbs = nullptr);
    ~TcpClient();
    bool Connect();
    void Disconnect();
    bool Send(const string& msg);
    void SetTcpCallbacks(const TcpCallbacksPtr& tcp_evt_cbs);
    TcpConnectionPtr& Connection() { return conn_; }
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
    bool                auto_reconnect_;
    IPAddress           server_addr_;
    TcpConnectionPtr    conn_;
    list<string>        tmp_sendbuf_list_;
    ReconnectTimer      reconnect_timer_;

    TcpCallbacksPtr     tcp_evt_cbs_;
};

}  // namespace evt_loop

#endif  // _TCP_CLIENT_H
