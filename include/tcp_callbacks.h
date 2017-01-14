#ifndef _TCP_CALLBACKS_H
#define _TCP_CALLBACKS_H

#include <stdio.h>
#include <functional>
#include <memory>

namespace evt_loop {

class TcpServer;
class TcpClient;
class TcpConnection;
class Message;

typedef std::function<void (TcpServer*, int, const char*) >         OnServerErrorCallback;
typedef std::function<void (TcpClient*, int, const char*) >         OnClientErrorCallback;
typedef std::function<void (TcpConnection*) >                       OnNewClientCallback;
typedef std::function<void (TcpConnection*) >                       OnReadyCallback;

typedef std::function<void (TcpConnection*, const Message*) >       OnMsgRecvdCallback;
typedef std::function<void (TcpConnection*, const Message*) >       OnMsgSentCallback;
typedef std::function<void (TcpConnection*) >                       OnClosedCallback;
typedef std::function<void (TcpConnection*, int, const char*) >     OnErrorCallback;

struct TcpCallbacks {
    public:
    TcpCallbacks() :
        on_msg_recvd_cb(std::bind(&TcpCallbacks::EmptyMsgRecvdCb, this, std::placeholders::_1, std::placeholders::_2)),
        on_msg_sent_cb(std::bind(&TcpCallbacks::EmptyMsgSentCb, this, std::placeholders::_1, std::placeholders::_2)),
        on_closed_cb(std::bind(&TcpCallbacks::EmptyClosedCb, this, std::placeholders::_1)),
        on_error_cb(std::bind(&TcpCallbacks::EmptyErrorCb, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)),
        on_conn_ready_cb(std::bind(&TcpCallbacks::EmptyReadyCb, this, std::placeholders::_1))
    { }

    public:
    OnMsgRecvdCallback  on_msg_recvd_cb;
    OnMsgSentCallback   on_msg_sent_cb;
    OnClosedCallback    on_closed_cb;
    OnErrorCallback     on_error_cb;
    OnReadyCallback     on_conn_ready_cb;

    private:
    void EmptyMsgRecvdCb(TcpConnection*, const Message*)        { printf("Empty Message Received Callback\n"); }
    void EmptyMsgSentCb(TcpConnection*, const Message*)         { printf("Empty Message Sent Callback\n"); }
    void EmptyClosedCb(TcpConnection*)                          { printf("Empty Connection Closed Callback\n"); }
    void EmptyErrorCb(TcpConnection*, int, const char*)         { printf("Empty Connection Error Callback\n"); }
    void EmptyReadyCb(TcpConnection*)                           { printf("Empty Connection Ready Callback\n"); }
};

typedef std::shared_ptr<TcpCallbacks>           TcpCallbacksPtr;

}  // namespace evt_loop

#endif  // _TCP_CALLBACKS_H
