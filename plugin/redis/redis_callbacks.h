#ifndef _REDIS_CALLBACKS_H
#define _REDIS_CALLBACKS_H

#include <stdio.h>
#include <functional>
#include <memory>

namespace hiredis {

class RedisAsyncClient;
class RedisMessage;

typedef std::function<void (RedisAsyncClient*, const RedisMessage*) >       OnReplyCallback;
typedef std::function<void (RedisAsyncClient*, const RedisMessage*) >       OnCmdSentCallback;
typedef std::function<void (RedisAsyncClient*) >                            OnConnectedCallback;
typedef std::function<void (RedisAsyncClient*) >                            OnClosedCallback;
typedef std::function<void (int, const char*) >                             OnErrorCallback;

struct RedisCallbacks {
    public:
    RedisCallbacks() :
        on_reply_cb(std::bind(&RedisCallbacks::EmptyReplyCb, this, std::placeholders::_1, std::placeholders::_2)),
        on_cmd_sent_cb(std::bind(&RedisCallbacks::EmptyCmdSentCb, this, std::placeholders::_1, std::placeholders::_2)),
        on_connected_cb(std::bind(&RedisCallbacks::EmptyConnectedCb, this, std::placeholders::_1)),
        on_closed_cb(std::bind(&RedisCallbacks::EmptyClosedCb, this, std::placeholders::_1)),
        on_error_cb(std::bind(&RedisCallbacks::EmptyErrorCb, this, std::placeholders::_1, std::placeholders::_2))
    { }

    public:
    OnReplyCallback     on_reply_cb;
    OnCmdSentCallback   on_cmd_sent_cb;
    OnConnectedCallback on_connected_cb;
    OnClosedCallback    on_closed_cb;
    OnErrorCallback     on_error_cb;

    private:
    void EmptyReplyCb(RedisAsyncClient*, const RedisMessage*)           { printf("Empty Redis Reply Callback\n"); }
    void EmptyCmdSentCb(RedisAsyncClient*, const RedisMessage*)         { printf("Empty Redis Command Sent Callback\n"); }
    void EmptyConnectedCb(RedisAsyncClient*)                            { printf("Empty Connected Callback\n"); }
    void EmptyClosedCb(RedisAsyncClient*)                               { printf("Empty Connection Closed Callback\n"); }
    void EmptyErrorCb(int, const char*)                                 { printf("Empty Connection Error Callback\n"); }
};

typedef std::shared_ptr<RedisCallbacks>           RedisCallbacksPtr;

}  // namespace hiredis

#endif  // _REDIS_CALLBACKS_H
