#include <stdio.h>

#include "el.h"
#include "mqtt_client.h"

using namespace evt_loop;
using namespace mqtt_api;

class MqttClientTest
{
  public:
    MqttClientTest() : mqtt_client_("localhost", 8883, "mqtt_test"),
      publish_timer(TimeVal(10, 0), std::bind(&MqttClientTest::OnPushlishTimer, this, std::placeholders::_1))
    {
      MqttCallbacksPtr mqtt_cbs = std::make_shared<MqttCallbacks>();
      mqtt_cbs->on_connected_cb = std::bind(&MqttClientTest::OnMqttConnected, this, std::placeholders::_1);
      mqtt_cbs->on_disconnected_cb = std::bind(&MqttClientTest::OnMqttDisconnected, this, std::placeholders::_1);
      mqtt_cbs->on_subscribe_cb = std::bind(&MqttClientTest::OnMqttSubscribe, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
      mqtt_cbs->on_unsubscribe_cb = std::bind(&MqttClientTest::OnMqttUnsubscribe, this, std::placeholders::_1, std::placeholders::_2);
      mqtt_cbs->on_publish_cb = std::bind(&MqttClientTest::OnMqttPublish, this, std::placeholders::_1, std::placeholders::_2);
      mqtt_cbs->on_message_cb = std::bind(&MqttClientTest::OnMqttMessage, this, std::placeholders::_1, std::placeholders::_2);
      mqtt_cbs->on_error_cb = std::bind(&MqttClientTest::OnMqttError, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
      mqtt_client_.SetCallbacks(mqtt_cbs);

      mqtt_client_.EnableTLS("/tmp/ca.crt");

      bool success = mqtt_client_.Connect();
      if (success) {
        printf("mqtt client call connect success\n");
      } else {
        printf("mqtt client connect failed\n");
      }

      mqtt_client_.SetClientUsernamePassword("my_mqtt_user", "123456");
    }

    struct mosquitto* MosquittoClient() { return (struct mosquitto*)mqtt_client_.MosquittoClient(); }

  protected:
    void OnMqttConnected(MqttClient* mqtt)
    {
      printf("[OnMqttConnected]\n");
      int msgid = 123;
      int qos = 1;
      bool success = mqtt_client_.Subscribe(&msgid, "test_", qos);
      if (!success) {
        printf("[OnMqttConnected] Subscribe failed\n");
      }
      publish_timer.Start();
    }
    void OnMqttDisconnected(MqttClient* mqtt)
    {
      printf("OnMqttDisconnected\n");
    }
    void OnMqttSubscribe(MqttClient* mqtt, int msgid, const GrantedQos* granted_qos)
    {
      printf("OnMqttSubscribe\n");
    }
    void OnMqttUnsubscribe(MqttClient* mqtt, int msgid)
    {
      printf("OnMqttUnsubscribe\n");
    }
    void OnMqttPublish(MqttClient* mqtt, int msgid)
    {
      printf("OnMqttPublish\n");
    }
    void OnMqttMessage(MqttClient* mqtt, const MqttMessage* msg)
    {
      printf("OnMqttMessage: topic: %s, msg: %s\n", msg->MosqMessage()->topic, (char*)msg->MosqMessage()->payload);
    }
    void OnMqttError(MqttClient* mqtt, int errcode, const char* errstr)
    {
      printf("OnMqttError: errcode: %d, errstr: %s\n", errcode, errstr);
    }

    void OnPushlishTimer(TimerEvent* timer)
    {
        printf("[OnPushlishTimer] publish message\n");
        int msgid = 123;
        int qos = 1;
        bool success = mqtt_client_.Publish(&msgid, "test_pub_", 5, "hello", qos, false);
        if (!success) {
          printf("[OnPushlishTimer] publish failed\n");
        }
    }

  private:
    MqttClient mqtt_client_;
    PeriodicTimer publish_timer;
};

int main(int argc, char **argv)
{
  MqttClientTest mqtt_client_test;

#if 1
  EV_Singleton->StartLoop();   // NOTE: not works in SSL mode
#else
  mosquitto_loop_forever(mqtt_client_test.MosquittoClient(), -1, 1);
#endif

  return 0;
}
