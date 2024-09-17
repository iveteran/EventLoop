#include "mqtt_callbacks.h"
#include "core/logger.h"

using namespace evt_loop;

namespace mqtt_api {

void MqttCallbacks::EmptyMessageCb(MqttClient*, const MqttMessage*) {
    el_logger->debug("Empty MqttClient Message Reply Callback");
}
void MqttCallbacks::EmptySubscribeCb(MqttClient*, int msgid, const GrantedQos*) {
    el_logger->debug("Empty MqttClient Subscribe Reply Callback");
}
void MqttCallbacks::EmptyUnsubscribeCb(MqttClient*, int msgid) {
    el_logger->debug("Empty MqttClient Unsubscribe Reply Callback");
}
void MqttCallbacks::EmptyPublishCb(MqttClient*, int msgid) {
    el_logger->debug("Empty MqttClient Publish Reply Callback");
}
void MqttCallbacks::EmptyConnectedCb(MqttClient*) {
    el_logger->debug("Empty Connected Callback");
}
void MqttCallbacks::EmptyDisconnectedCb(MqttClient*) {
    el_logger->debug("Empty Disconnected Callback");
}
void MqttCallbacks::EmptyErrorCb(MqttClient*, int, const char*) {
    el_logger->debug("Empty Error Callback");
}

}  // namespace mqtt_api
