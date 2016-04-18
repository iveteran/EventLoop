#ifndef _REDIS_CALLBACKS_H
#define _REDIS_CALLBACKS_H

#include <stdio.h>
#include <functional>
#include <memory>

namespace evt_loop {

class RedisAsyncClient;
class RedisMessage;

typedef std::function<void (RedisAsyncClient*, const RedisMessage*) >       OnMsgRecvdCallback;
typedef std::function<void (RedisAsyncClient*, const RedisMessage*) >       OnMsgSentCallback;
typedef std::function<void (RedisAsyncClient*) >                            OnNewClientCallback;
typedef std::function<void (RedisAsyncClient*) >                            OnClosedCallback;
typedef std::function<void (int, const char*) >                             OnErrorCallback;

struct RedisCallbacks {
    public:
    RedisCallbacks() :
        on_msg_recvd_cb(std::bind(&RedisCallbacks::EmptyMsgRecvdCb, this, std::placeholders::_1, std::placeholders::_2)),
        on_msg_sent_cb(std::bind(&RedisCallbacks::EmptyMsgSentCb, this, std::placeholders::_1, std::placeholders::_2)),
        on_new_client_cb(std::bind(&RedisCallbacks::EmptyNewClientCb, this, std::placeholders::_1)),
        on_closed_cb(std::bind(&RedisCallbacks::EmptyClosedCb, this, std::placeholders::_1)),
        on_error_cb(std::bind(&RedisCallbacks::EmptyErrorCb, this, std::placeholders::_1, std::placeholders::_2))
    { }

    public:
    OnMsgRecvdCallback  on_msg_recvd_cb;
    OnMsgSentCallback   on_msg_sent_cb;
    OnNewClientCallback on_new_client_cb;
    OnClosedCallback    on_closed_cb;
    OnErrorCallback     on_error_cb;

    private:
    void EmptyMsgRecvdCb(RedisAsyncClient*, const RedisMessage*)        { printf("Empty RedisMessage Received Callback\n"); }
    void EmptyMsgSentCb(RedisAsyncClient*, const RedisMessage*)         { printf("Empty RedisMessage Sent Callback\n"); }
    void EmptyNewClientCb(RedisAsyncClient*)                            { printf("Empty New Client Callback\n"); }
    void EmptyClosedCb(RedisAsyncClient*)                               { printf("Empty Connection Closed Callback\n"); }
    void EmptyErrorCb(int, const char*)                                 { printf("Empty Connection Error Callback\n"); }
};

typedef std::shared_ptr<RedisCallbacks>           RedisCallbacksPtr;

}  // namespace evt_loop

#endif  // _REDIS_CALLBACKS_H
