#include "mqtt_callbacks.h"
#include "core/logger.h"

using namespace evt_loop;

namespace mqtt_api {

void MqttCallbacks::EmptyMessageCb(MqttClient*, const MqttMessage*) {
    el_logger->debug("Empty MqttClient Message Reply Callback\n");
}
void MqttCallbacks::EmptySubscribeCb(MqttClient*, int msgid, const GrantedQos*) {
    el_logger->debug("Empty MqttClient Subscribe Reply Callback\n");
}
void MqttCallbacks::EmptyUnsubscribeCb(MqttClient*, int msgid) {
    el_logger->debug("Empty MqttClient Unsubscribe Reply Callback\n");
}
void MqttCallbacks::EmptyPublishCb(MqttClient*, int msgid) {
    el_logger->debug("Empty MqttClient Publish Reply Callback\n");
}
void MqttCallbacks::EmptyConnectedCb(MqttClient*) {
    el_logger->debug("Empty Connected Callback\n");
}
void MqttCallbacks::EmptyDisconnectedCb(MqttClient*) {
    el_logger->debug("Empty Disconnected Callback\n");
}
void MqttCallbacks::EmptyErrorCb(MqttClient*, int, const char*) {
    el_logger->debug("Empty Error Callback\n");
}

}  // namespace mqtt_api
