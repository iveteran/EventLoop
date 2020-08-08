#include "mqtt_client.h"

namespace mqtt_api {

void _mosq_log_callback(struct mosquitto *mosq, void *userdata, int level, const char *str)
{
  /* Pring all log messages regardless of level. */
  printf("[_mosq_log_callback] level: %d, log: %s\n", level, str);
  MqttClient* mqtt_client = (MqttClient*)userdata;
  (void)mqtt_client;  // disable gcc warning
}

void _mosq_connect_callback(struct mosquitto *mosq, void *userdata, int rc)
{
  MqttClient* mqtt_client = (MqttClient*)userdata;
  if (!rc) {
    printf("[_mosq_connect_callback] Connect success (rc: %d)\n", rc);
    mqtt_client->OnConnected();
  } else {
    fprintf(stderr, "[_mosq_connect_callback] Connect failed (rc: %d)\n", rc);
    mqtt_client->OnError(rc, mosquitto_strerror(rc));
  }
}

void _mosq_disconnect_callback(struct mosquitto *mosq, void *userdata, int rc)
{
  fprintf(stderr, "[_mosq_disconnect_callback] Disconnect (rc: %d)\n", rc);
  MqttClient* mqtt_client = (MqttClient*)userdata;
  if (rc) {
    mqtt_client->OnError(rc, mosquitto_strerror(rc));
  }
  mqtt_client->OnDisconnected();
}

void _mosq_subscribe_callback(struct mosquitto *mosq, void *userdata, int msgid, int qos_count, const int *granted_qos)
{
  MqttClient* mqtt_client = (MqttClient*)userdata;
  printf("[_mosq_subscribe_callback] Subscribed (msgid: %d): %d\n", msgid, granted_qos[0]);
  GrantedQos qos_vector;
  for (int i=1; i<qos_count; i++){
    //printf(", %d", granted_qos[i]);
    qos_vector.granted_qos_.push_back(granted_qos[i]);
  }
  mqtt_client->OnSubscribe(msgid, &qos_vector);
}

void _mosq_unsubscribe_callback(struct mosquitto *mosq, void *userdata, int msgid)
{
  printf("[_mosq_unsubscribe_callback] Unsubscribed (msgid: %d)\n", msgid);
  MqttClient* mqtt_client = (MqttClient*)userdata;
  mqtt_client->OnUnsubscribe(msgid);
}

void _mosq_publish_callback(struct mosquitto *mosq, void *userdata, int msgid)
{
  printf("[_mosq_publish_callback] Published (msgid: %d)\n", msgid);
  MqttClient* mqtt_client = (MqttClient*)userdata;
  mqtt_client->OnPublish(msgid);
}

void _mosq_message_callback(struct mosquitto *mosq, void *userdata, const struct mosquitto_message *message)
{
  printf("[_mosq_message_callback] Message (topic: %s, message length: %d)\n", message->topic, message->payloadlen);
  MqttClient* mqtt_client = (MqttClient*)userdata;
  MqttMessage mqtt_msg((struct mosquitto_message *)message);
  mqtt_client->OnMessage(&mqtt_msg);
}

bool MqttClient::Init(const char* host, int port, const char* id, bool clean_session)
{
  bool success = false;
  mosquitto_lib_init();
  mosq_ = mosquitto_new(id, clean_session, this);
  if (mosq_ != NULL) {
    mosquitto_log_callback_set(mosq_, _mosq_log_callback);
    mosquitto_connect_callback_set(mosq_, _mosq_connect_callback);
    mosquitto_disconnect_callback_set(mosq_, _mosq_disconnect_callback);
    mosquitto_subscribe_callback_set(mosq_, _mosq_subscribe_callback);
    mosquitto_unsubscribe_callback_set(mosq_, _mosq_unsubscribe_callback);
    mosquitto_publish_callback_set(mosq_, _mosq_publish_callback);
    mosquitto_message_callback_set(mosq_, _mosq_message_callback);

    mosquitto_reconnect_delay_set(mosq_, 2, 10, false);
    success = true;
  } else {
    perror(mosquitto_strerror(errno));
  }
  return success;
}

void MqttClient::OnEvents(uint32_t events)
{
  //printf("[OnEvents] events: %d\n", events);
  if (events & FileEvent::WRITE) {
    //printf(">>>> [OnEvents] write event: %d\n", events);
    int status = mosquitto_loop_write(mosq_, MOSQ_MAX_PACKETS);
    if (status != MOSQ_ERR_SUCCESS) {
      OnError(status, mosquitto_strerror(errno));
    }
  }
  if (events & FileEvent::READ) {
    //printf(">>>> [OnEvents] read event: %d\n", events);
    int status = mosquitto_loop_read(mosq_, MOSQ_MAX_PACKETS);
    if (status != MOSQ_ERR_SUCCESS) {
      OnError(status, mosquitto_strerror(errno));
    }
  }
  if (events & FileEvent::CLOSED) {
    HandleDisconnect();
  }

  if (events & FileEvent::ERROR) {
    OnError(errno, strerror(errno));
  }
}

void MqttClient::OnReconnectTimer(TimerEvent* timer)
{
  //printf("[MqttClient::OnReconnectTimer begin] is ready: %d\n", IsReady());
  if (!IsReady()) {  // if the connection is not created, then reconnect
    bool success = Reconnect_();
    if (success)
      timer->Stop();
    else
      printf("[MqttClient::OnReconnectTimer] Reconnect failed, retry %u seconds later...\n", timer->GetInterval().Seconds());
  } else {
    timer->Stop();
  }
}

}  // namespace mqtt_api
