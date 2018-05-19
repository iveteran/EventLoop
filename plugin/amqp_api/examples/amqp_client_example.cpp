#include <stdio.h>

#include "el.h"
#include "amqp_client.h"
#include "amqp_callbacks.h"

using namespace evt_loop;
using namespace amqp_api;

const char* EXCHANGE_NAME = "hello-exchange";
const char* QUEUE_NAME = "hello-queue";
const char* ROUTING_KEY = "hola";

class AMQPClientTest
{
  public:
    AMQPClientTest() : amqp_client_("amqp://guest:guest@localhost/"),
      publish_timer(TimeVal(10, 0), std::bind(&AMQPClientTest::OnPushlishTimer, this, std::placeholders::_1))
    {
      AMQPCallbacksPtr amqp_cbs = std::make_shared<AMQPCallbacks>();
      amqp_cbs->on_connected_cb = std::bind(&AMQPClientTest::OnAMQPConnected, this, std::placeholders::_1);
      amqp_cbs->on_disconnected_cb = std::bind(&AMQPClientTest::OnAMQPDisconnected, this, std::placeholders::_1);
      amqp_cbs->on_subscribe_cb = std::bind(&AMQPClientTest::OnAMQPSubscribe, this, std::placeholders::_1, std::placeholders::_2);
      amqp_cbs->on_unsubscribe_cb = std::bind(&AMQPClientTest::OnAMQPUnsubscribe, this, std::placeholders::_1, std::placeholders::_2);
      amqp_cbs->on_publish_cb = std::bind(&AMQPClientTest::OnAMQPPublish, this, std::placeholders::_1, std::placeholders::_2);
      amqp_cbs->on_message_cb = std::bind(&AMQPClientTest::OnAMQPMessage, this, std::placeholders::_1, std::placeholders::_2);
      amqp_cbs->on_error_cb = std::bind(&AMQPClientTest::OnAMQPError, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
      amqp_client_.SetCallbacks(amqp_cbs);

    }

  protected:
    void BuildQueue()
    {
      amqp_client_.DeclareExchange(EXCHANGE_NAME, AMQP::ExchangeType::direct, AMQP::durable);
      amqp_client_.DeclareQueue(QUEUE_NAME);
      amqp_client_.BindQueue(EXCHANGE_NAME, QUEUE_NAME, ROUTING_KEY);
    }

    void OnAMQPConnected(AMQPClient* amqp_client)
    {
      printf("[AMQPClientTest::OnAMQPConnected]\n");

      BuildQueue();

      amqp_client_.Consume(QUEUE_NAME);
      publish_timer.Start();
    }
    void OnAMQPDisconnected(AMQPClient* amqp_client)
    {
      printf("[AMQPClientTest::OnAMQPDisconnected]\n");
      publish_timer.Stop();
    }
    void OnAMQPSubscribe(AMQPClient* amqp_client, int msgid)
    {
      printf("[AMQPClientTest::OnAMQPSubscribe]\n");
    }
    void OnAMQPUnsubscribe(AMQPClient* amqp_client, int msgid)
    {
      printf("[AMQPClientTest::OnAMQPUnsubscribe]\n");
    }
    void OnAMQPPublish(AMQPClient* amqp_client, int msgid)
    {
      printf("[AMQPClientTest::OnAMQPPublish]\n");
    }
    void OnAMQPMessage(AMQPClient* amqp_client, const AMQPMessage* amqp_msg)
    {
      const string msg(amqp_msg->Data(), amqp_msg->DataLength());
      printf("[AMQPClientTest::OnAMQPMessage] Received message, routing key: %s, msg: %s\n", amqp_msg->RoutingKey().c_str(), msg.c_str());
    }
    void OnAMQPError(AMQPClient* amqp_client, int errcode, const char* errstr)
    {
      printf("[AMQPClientTest::OnAMQPError] errcode: %d, errstr: %s\n", errcode, errstr);
    }

    void OnPushlishTimer(TimerEvent* timer)
    {
        const char* message = "hello world";
        printf("[AMQPClientTest::OnPushlishTimer] publish message: %s\n", message);
        amqp_client_.Publish(EXCHANGE_NAME, ROUTING_KEY, message);
    }

  private:
    AMQPClient amqp_client_;
    PeriodicTimer publish_timer;
};

int main(int argc, char **argv)
{
  AMQPClientTest AMQP_client_test;

  EV_Singleton->StartLoop();

  return 0;
}
