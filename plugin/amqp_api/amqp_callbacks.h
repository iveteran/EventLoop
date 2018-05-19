#ifndef _AMQP_CALLBACKS_H
#define _AMQP_CALLBACKS_H

#include <stdio.h>
#include <functional>
#include <memory>

namespace amqp_api {

class AMQPClient;
class AMQPMessage;

typedef std::function<void (AMQPClient*, const AMQPMessage*) >      OnMessageCallback;
typedef std::function<void (AMQPClient*, int msgid) >               OnSubscribeCallback;
typedef std::function<void (AMQPClient*, int msgid) >               OnUnsubscribeCallback;
typedef std::function<void (AMQPClient*, int msgid) >               OnPublishCallback;
typedef std::function<void (AMQPClient*) >                          OnConnectedCallback;
typedef std::function<void (AMQPClient*) >                          OnDisconnectedCallback;
typedef std::function<void (AMQPClient*, int, const char*) >        OnErrorCallback;

struct AMQPCallbacks {
    public:
    AMQPCallbacks() :
        on_message_cb(std::bind(&AMQPCallbacks::EmptyMessageCb, this, std::placeholders::_1, std::placeholders::_2)),
        on_subscribe_cb(std::bind(&AMQPCallbacks::EmptySubscribeCb, this, std::placeholders::_1, std::placeholders::_2)),
        on_unsubscribe_cb(std::bind(&AMQPCallbacks::EmptyUnsubscribeCb, this, std::placeholders::_1, std::placeholders::_2)),
        on_publish_cb(std::bind(&AMQPCallbacks::EmptyPublishCb, this, std::placeholders::_1, std::placeholders::_2)),
        on_connected_cb(std::bind(&AMQPCallbacks::EmptyConnectedCb, this, std::placeholders::_1)),
        on_disconnected_cb(std::bind(&AMQPCallbacks::EmptyDisconnectedCb, this, std::placeholders::_1)),
        on_error_cb(std::bind(&AMQPCallbacks::EmptyErrorCb, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3))
    { }

    public:
    OnMessageCallback       on_message_cb;
    OnSubscribeCallback     on_subscribe_cb;
    OnUnsubscribeCallback   on_unsubscribe_cb;
    OnPublishCallback       on_publish_cb;
    OnConnectedCallback     on_connected_cb;
    OnDisconnectedCallback  on_disconnected_cb;
    OnErrorCallback         on_error_cb;

    private:
    void EmptyMessageCb(AMQPClient*, const AMQPMessage*)               { printf("Empty AMQPClient Message Reply Callback\n"); }
    void EmptySubscribeCb(AMQPClient*, int msgid)                      { printf("Empty AMQPClient Subscribe Reply Callback\n"); }
    void EmptyUnsubscribeCb(AMQPClient*, int msgid)                    { printf("Empty AMQPClient Unsubscribe Reply Callback\n"); }
    void EmptyPublishCb(AMQPClient*, int msgid)                        { printf("Empty AMQPClient Publish Reply Callback\n"); }
    void EmptyConnectedCb(AMQPClient*)                          { printf("Empty Connected Callback\n"); }
    void EmptyDisconnectedCb(AMQPClient*)                       { printf("Empty Disconnected Callback\n"); }
    void EmptyErrorCb(AMQPClient*, int, const char*)            { printf("Empty Error Callback\n"); }
};

typedef std::shared_ptr<AMQPCallbacks>      AMQPCallbacksPtr;
typedef std::queue<OnMessageCallback>       AMQPMessageCallbackQueue;

}  // namespace amqp_api

#endif  // _AMQP_CALLBACKS_H
