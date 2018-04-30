#ifndef __MQTT_mosq_H
#define __MQTT_mosq_H

#include <mosquitto.h>
#include "el.h"
#include "mqtt_callbacks.h"

#define MOSQ_MAX_PACKETS 1

using namespace evt_loop;

namespace mqtt_api {

void _mosq_log_callback(struct mosquitto *mosq, void *userdata, int level, const char *str);
void _mosq_connect_callback(struct mosquitto *mosq, void *userdata, int rc);
void _mosq_disconnect_callback(struct mosquitto *mosq, void *userdata, int rc);
void _mosq_subscribe_callback(struct mosquitto *mosq, void *userdata, int msgid, int qos_count, const int *granted_qos);
void _mosq_unsubscribe_callback(struct mosquitto *mosq, void *userdata, int msgid);
void _mosq_publish_callback(struct mosquitto *mosq, void *userdata, int msgid);
void _mosq_message_callback(struct mosquitto *mosq, void *userdata, const struct mosquitto_message *message);

class GrantedQos {
  public:
    std::vector<int> granted_qos_;
};

class MqttMessage {
  public:
    MqttMessage(struct mosquitto_message* mosq_msg) : mosq_msg_(mosq_msg) { }
    MqttMessage& operator=(const MqttMessage& other)
    {
      mosquitto_message_free(&mosq_msg_);
      mosquitto_message_copy(mosq_msg_, other.mosq_msg_);
      return *this;
    }
    MqttMessage& operator=(const struct mosquitto_message* mosq_msg)
    {
      mosquitto_message_free(&mosq_msg_);
      mosquitto_message_copy(mosq_msg_, mosq_msg_);
      return *this;
    }

    const struct mosquitto_message* MosqMessage() const { return mosq_msg_; }

    ~MqttMessage()
    {
      //mosquitto_message_free(&mosq_msg_);
    }

  private:
    struct mosquitto_message* mosq_msg_;
};

class MqttClient : public IOEvent {
  public:
    friend void _mosq_log_callback(struct mosquitto *mosq, void *userdata, int level, const char *str);
    friend void _mosq_connect_callback(struct mosquitto *mosq, void *userdata, int rc);
    friend void _mosq_disconnect_callback(struct mosquitto *mosq, void *userdata, int rc);
    friend void _mosq_subscribe_callback(struct mosquitto *mosq, void *userdata, int msgid, int qos_count, const int *granted_qos);
    friend void _mosq_unsubscribe_callback(struct mosquitto *mosq, void *userdata, int msgid);
    friend void _mosq_publish_callback(struct mosquitto *mosq, void *userdata, int msgid);
    friend void _mosq_message_callback(struct mosquitto *mosq, void *userdata, const struct mosquitto_message *message);

  public:
    MqttClient(const char* host, int port, const char* id = NULL, bool clean_session = true)
      : mosq_(NULL), host_(host), port_(port), keepalive_(60), connected_(false), auto_reconnect_(true),
      reconnect_timer_(std::bind(&MqttClient::OnReconnectTimer, this, std::placeholders::_1)),
      mosq_loop_task_(NULL)
    {
      Init(host, port, id, clean_session);

      if (auto_reconnect_) {
        reconnect_timer_.SetInterval(TimeVal(1, 0));
      }
    }

    bool Init(const char* host, int port, const char* id = NULL, bool clean_session = true);

    void SetCallbacks(const MqttCallbacksPtr& callbacks) { mqtt_callbacks_ = callbacks; }

    virtual ~MqttClient()
    {
      mosquitto_destroy(mosq_);
      mosquitto_lib_cleanup();
    }

    bool EnableTLS(const char* cafile, const char* capath, const char* certfile, const char* keyfile)
    {
      int status = mosquitto_tls_set(mosq_, cafile, capath, certfile, keyfile, NULL);
      //status = mosquitto_tls_insecure_set(mosq_, true);
      return status == MOSQ_ERR_SUCCESS;
    }

    bool SetClientUsernamePassword(const char* username, const char* password)
    {
      int status = mosquitto_username_pw_set(mosq_, username, password);
      return status == MOSQ_ERR_SUCCESS;
    }

    const struct mosquitto* MosquittoClient() const { return mosq_; }

    bool IsReady() const { return connected_; }

    bool Connect()
    {
      int status = mosquitto_connect(mosq_, host_.c_str(), port_, keepalive_);
      bool success = (status == MOSQ_ERR_SUCCESS);
      if (!success && auto_reconnect_) {
        Reconnect();
      } else {
        delete mosq_loop_task_;
        mosq_loop_task_ = new TickEvent(std::bind(&MqttClient::ProcessMosquittoLoop, this, std::placeholders::_1, std::placeholders::_2), this, 1);
      }
      return success;
    }

    void Reconnect()
    {
      //Disconnect();
      if (!reconnect_timer_.IsRunning())
        reconnect_timer_.Start();
    }
    
    bool Disconnect()
    {
      int status = mosquitto_disconnect(mosq_);
      if (status == MOSQ_ERR_SUCCESS) {
        HandleDisconnect();
        return true;
      } else {
        return false;
      }
    }

    bool Subscribe(int* msgid, const char* sub_patten, int qos)
    {
      if (!connected_) return false;
      int status = mosquitto_subscribe(mosq_, msgid, sub_patten, qos);
      return status == MOSQ_ERR_SUCCESS;
    }

    bool Publish(int* msgid, const char* topic, int msglen, const char* msg, int qos, bool retain)
    {
      if (!connected_) return false;
      int status = mosquitto_publish(mosq_, msgid, topic, msglen, msg, qos, retain);
      return status == MOSQ_ERR_SUCCESS;
    }

  protected:
    bool Reconnect_()
    {
      int status = mosquitto_reconnect(mosq_);
      bool success = (status == MOSQ_ERR_SUCCESS);
      if (success ) {
        delete mosq_loop_task_;
        mosq_loop_task_ = new TickEvent(std::bind(&MqttClient::ProcessMosquittoLoop, this, std::placeholders::_1, std::placeholders::_2), this, 1);
      }
      return success;
    }

    void OnEvents(uint32_t events) override;

    void OnError(int errcode, const char* errstr)
    {
      mqtt_callbacks_->on_error_cb(this, errcode, errstr);
    }

    void OnConnected()
    {
      HandleConnect();
      mqtt_callbacks_->on_connected_cb(this);
    }

    void OnDisconnected()
    {
      HandleDisconnect();
      mqtt_callbacks_->on_disconnected_cb(this);
      if (auto_reconnect_) {
        Reconnect();
      }
    }

    void OnSubscribe(int msgid, const GrantedQos* qranted_qos)
    {
      mqtt_callbacks_->on_subscribe_cb(this, msgid, qranted_qos);
    }

    void OnUnsubscribe(int msgid)
    {
      mqtt_callbacks_->on_unsubscribe_cb(this, msgid);
    }

    void OnPublish(int msgid)
    {
      mqtt_callbacks_->on_publish_cb(this, msgid);
    }

    void OnMessage(const MqttMessage* mqtt_msg)
    {
      mqtt_callbacks_->on_message_cb(this, mqtt_msg);
    }

    void HandleConnect()
    {
      connected_ = true;
      int fd = mosquitto_socket(mosq_);
      SetFD(fd);

      delete mosq_loop_task_;
      mosq_loop_task_ = NULL;
      //DeleteWriteEvent();
    }

    void HandleDisconnect()
    {
      SetFD(-1);
      connected_ = false;
    }

  private:
    void ProcessMosquittoLoop(UserEvent* tick_events, void* udata)
    {
      printf("[ProcessMosquittoLoop] Trigger tick event(id: %d), udata: %p\n", tick_events->Id(), udata);
      //MqttClient* mqtt_client = (MqttClient*)udata;
      int status = mosquitto_loop(mosq_, -1, 1);
      printf("[ProcessMosquittoLoop] status: %s\n", mosquitto_strerror(status));
    }

    void OnReconnectTimer(TimerEvent* timer);

  private:
    struct mosquitto* mosq_;
    string host_;
    int port_;
    int keepalive_;
    bool connected_;
    bool auto_reconnect_;
    PeriodicTimer reconnect_timer_;
    MqttCallbacksPtr mqtt_callbacks_;
    TickEvent* mosq_loop_task_;
};

}

#endif  // __MQTT_mosq_H
