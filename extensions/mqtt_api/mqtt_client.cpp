#include "mqtt_client.h"
#include "core/logger.h"

namespace mqtt_api {

void _mosq_log_callback(struct mosquitto *mosq, void *userdata, int level, const char *str)
{
  /* Pring all log messages regardless of level. */
  el_logger->debug("[_mosq_log_callback] level: {}, log: {}", level, str);
  MqttClient* mqtt_client = (MqttClient*)userdata;
  (void)mqtt_client;  // disable gcc warning
}

void _mosq_connect_callback(struct mosquitto *mosq, void *userdata, int rc)
{
  MqttClient* mqtt_client = (MqttClient*)userdata;
  if (!rc) {
    el_logger->debug("[_mosq_connect_callback] Connect success (rc: {})", rc);
    mqtt_client->OnConnected();
  } else {
    el_logger->error("[_mosq_connect_callback] Connect failed (rc: {})", rc);
    mqtt_client->OnError(rc, mosquitto_strerror(rc));
  }
}

void _mosq_disconnect_callback(struct mosquitto *mosq, void *userdata, int rc)
{
  el_logger->error("[_mosq_disconnect_callback] Disconnect (rc: {})", rc);
  MqttClient* mqtt_client = (MqttClient*)userdata;
  if (rc) {
    mqtt_client->OnError(rc, mosquitto_strerror(rc));
  }
  mqtt_client->OnDisconnected();
}

void _mosq_subscribe_callback(struct mosquitto *mosq, void *userdata, int msgid, int qos_count, const int *granted_qos)
{
  MqttClient* mqtt_client = (MqttClient*)userdata;
  el_logger->debug("[_mosq_subscribe_callback] Subscribed (msgid: {}): {}", msgid, granted_qos[0]);
  GrantedQos qos_vector;
  for (int i=1; i<qos_count; i++){
    //el_logger->output(", {}", granted_qos[i]);
    qos_vector.granted_qos_.push_back(granted_qos[i]);
  }
  mqtt_client->OnSubscribe(msgid, &qos_vector);
}

void _mosq_unsubscribe_callback(struct mosquitto *mosq, void *userdata, int msgid)
{
  el_logger->debug("[_mosq_unsubscribe_callback] Unsubscribed (msgid: {})", msgid);
  MqttClient* mqtt_client = (MqttClient*)userdata;
  mqtt_client->OnUnsubscribe(msgid);
}

void _mosq_publish_callback(struct mosquitto *mosq, void *userdata, int msgid)
{
  el_logger->debug("[_mosq_publish_callback] Published (msgid: {})", msgid);
  MqttClient* mqtt_client = (MqttClient*)userdata;
  mqtt_client->OnPublish(msgid);
}

void _mosq_message_callback(struct mosquitto *mosq, void *userdata, const struct mosquitto_message *message)
{
  el_logger->debug("[_mosq_message_callback] Message (topic: {}, message length: {})", message->topic, message->payloadlen);
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
  //el_logger->debug("[MqttClient::OnEvents] events: {}", events);
  if (events & FileEvent::WRITE) {
    //el_logger->debug(">>>> [MqttClient::OnEvents] write event: {}", events);
    int status = mosquitto_loop_write(mosq_, MOSQ_MAX_PACKETS);
    if (status != MOSQ_ERR_SUCCESS) {
      OnError(status, mosquitto_strerror(errno));
    }
  }
  if (events & FileEvent::READ) {
    //el_logger->debug(">>>> [MqttClient::OnEvents] read event: {}", events);
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

void MqttClient::ProcessMosquittoLoop(UserEvent* tick_events, void* udata)
{
  el_logger->debug("[MqttClient::ProcessMosquittoLoop] Trigger tick event(id: {}), udata: {}", tick_events->Id(), udata);
  //MqttClient* mqtt_client = (MqttClient*)udata;
  int status = mosquitto_loop(mosq_, -1, 1);
  el_logger->debug("[MqttClient::ProcessMosquittoLoop] status: {}", mosquitto_strerror(status));
}

void MqttClient::OnReconnectTimer(TimerEvent* timer)
{
  //el_logger->debug("[MqttClient::OnReconnectTimer begin] is ready: {}", IsReady());
  if (!IsReady()) {  // if the connection is not created, then reconnect
    bool success = Reconnect_();
    if (success)
      timer->Stop();
    else
      el_logger->error("[MqttClient::OnReconnectTimer] Reconnect failed, retry {} seconds later...", timer->GetInterval().Seconds());
  } else {
    timer->Stop();
  }
}

bool MqttClient::Connect()
{
  int status = mosquitto_connect(mosq_, host_.c_str(), port_, keepalive_);
  bool success = (status == MOSQ_ERR_SUCCESS);
  if (!success && auto_reconnect_) {
    Reconnect();
  } else {
    CreateMosqLoopTask();
  }
  return success;
}

bool MqttClient::Reconnect_()
{
  int status = mosquitto_reconnect(mosq_);
  bool success = (status == MOSQ_ERR_SUCCESS);
  if (success ) {
    CreateMosqLoopTask();
  }
  return success;
}

void MqttClient::CreateMosqLoopTask() {
    delete mosq_loop_task_;
    mosq_loop_task_ = new TickEvent(std::bind(&MqttClient::ProcessMosquittoLoop, this, std::placeholders::_1, std::placeholders::_2), this, 1);
}

void MqttClient::HandleConnect()
{
  connected_ = true;
  int fd = mosquitto_socket(mosq_);
  SetFD(fd);

  delete mosq_loop_task_;
  mosq_loop_task_ = NULL;
  //DeleteWriteEvent();
}

MqttClient::~MqttClient()
{
  mosquitto_destroy(mosq_);
  mosquitto_lib_cleanup();
  delete mosq_loop_task_;
}

}  // namespace mqtt_api
