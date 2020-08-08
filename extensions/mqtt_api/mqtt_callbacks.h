#ifndef _MQTT_CALLBACKS_H
#define _MQTT_CALLBACKS_H

#include <stdio.h>
#include <functional>
#include <memory>

namespace mqtt_api {

class MqttClient;
class MqttMessage;
class GrantedQos;

typedef std::function<void (MqttClient*, const MqttMessage*) >      OnMessageCallback;
typedef std::function<void (MqttClient*, int msgid, const GrantedQos*) >  OnSubscribeCallback;
typedef std::function<void (MqttClient*, int msgid) >               OnUnsubscribeCallback;
typedef std::function<void (MqttClient*, int msgid) >               OnPublishCallback;
typedef std::function<void (MqttClient*) >                          OnConnectedCallback;
typedef std::function<void (MqttClient*) >                          OnDisconnectedCallback;
typedef std::function<void (MqttClient*, int, const char*) >        OnErrorCallback;

struct MqttCallbacks {
    public:
    MqttCallbacks() :
        on_message_cb(std::bind(&MqttCallbacks::EmptyMessageCb, this, std::placeholders::_1, std::placeholders::_2)),
        on_subscribe_cb(std::bind(&MqttCallbacks::EmptySubscribeCb, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)),
        on_unsubscribe_cb(std::bind(&MqttCallbacks::EmptyUnsubscribeCb, this, std::placeholders::_1, std::placeholders::_2)),
        on_publish_cb(std::bind(&MqttCallbacks::EmptyPublishCb, this, std::placeholders::_1, std::placeholders::_2)),
        on_connected_cb(std::bind(&MqttCallbacks::EmptyConnectedCb, this, std::placeholders::_1)),
        on_disconnected_cb(std::bind(&MqttCallbacks::EmptyDisconnectedCb, this, std::placeholders::_1)),
        on_error_cb(std::bind(&MqttCallbacks::EmptyErrorCb, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3))
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
    void EmptyMessageCb(MqttClient*, const MqttMessage*)                      { printf("Empty MqttClient Message Reply Callback\n"); }
    void EmptySubscribeCb(MqttClient*, int msgid, const GrantedQos*)  { printf("Empty MqttClient Subscribe Reply Callback\n"); }
    void EmptyUnsubscribeCb(MqttClient*, int msgid)                    { printf("Empty MqttClient Unsubscribe Reply Callback\n"); }
    void EmptyPublishCb(MqttClient*, int msgid)                        { printf("Empty MqttClient Publish Reply Callback\n"); }
    void EmptyConnectedCb(MqttClient*)                          { printf("Empty Connected Callback\n"); }
    void EmptyDisconnectedCb(MqttClient*)                       { printf("Empty Disconnected Callback\n"); }
    void EmptyErrorCb(MqttClient*, int, const char*)            { printf("Empty Error Callback\n"); }
};

typedef std::shared_ptr<MqttCallbacks>      MqttCallbacksPtr;
typedef std::queue<OnMessageCallback>       MQTTMessageCallbackQueue;

}  // namespace mqtt_api

#endif  // _MQTT_CALLBACKS_H
