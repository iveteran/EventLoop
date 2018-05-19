#ifndef _AMQP_CLIENT_H
#define _AMQP_CLIENT_H 

#include "amqp_event_handler.h"
#include "amqp_callbacks.h"

namespace amqp_api {

using AMQPTcpConnectionPtr = std::shared_ptr<AMQP::TcpConnection>;
using AMQPChannelPtr = std::shared_ptr<AMQP::TcpChannel>;

class AMQPMessage
{
  public:
  AMQPMessage(const AMQP::Message* msg) : msg_(msg) { }

  const char* Data() const { return msg_->body(); }
  size_t DataLength() const { return msg_->bodySize(); }

  const string& RoutingKey() const { return msg_->routingkey(); }

  const AMQP::Message& amqp_message() const { return *msg_; }

  private:
  const AMQP::Message* msg_;
};

class AMQPClient : public AMQPEventHandler
{
  public:
    AMQPClient(const char* address, bool auto_reconnect = true) :
      amqp_address_(address), auto_reconnect_(auto_reconnect),
      reconnect_timer_(std::bind(&AMQPClient::OnReconnectTimer, this, std::placeholders::_1))
    {
      if (auto_reconnect_) {
        reconnect_timer_.SetInterval(TimeVal(1, 0));
      }

      BuildChannel();
    }
    bool IsReady() { return amqp_connection_->fileno() > 0; }

    void SetCallbacks(const AMQPCallbacksPtr& amqp_cbs) { amqp_callbacks_ = amqp_cbs; }

    AMQPChannelPtr AMQPChannel() { return channel_; }

    void DeclareExchange(const char* exchange, AMQP::ExchangeType exchangeType, int flag);
    void DeclareQueue(const char* queue_name);
    void BindQueue(const char* exchange, const char* queue, const char* routingKey);
    void Publish(const char* exchange, const char* routing_key, const char* message);
    void Consume(const char* queue_name);

  private:
    void BuildChannel()
    {
      amqp_connection_ = std::make_shared<AMQP::TcpConnection>(this, amqp_address_);
      channel_ = std::make_shared<AMQP::TcpChannel>(amqp_connection_.get());
    }
    void DestoryChannel()
    {
      channel_ = nullptr;
      amqp_connection_ = nullptr;
    }
    void Reconnect()
    {
      DestoryChannel();
      BuildChannel();
    }
    void StartReconnectTimer()
    {
      if (!reconnect_timer_.IsRunning())
        reconnect_timer_.Start();
    }
    void StopReconnectTimer()
    {
      if (reconnect_timer_.IsRunning())
        reconnect_timer_.Stop();
    }

    void OnReconnectTimer(TimerEvent* timer)
    {
      printf("[OnReconnectTimer begin] is ready: %d\n", IsReady());
      if (!IsReady()) {  // if the connection is not created, then reconnect
        Reconnect();
      } else {
        timer->Stop();
      }
    }

  protected:
    virtual uint16_t onNegotiate(AMQP::TcpConnection *connection, uint16_t interval) override
    {
      printf("[AMQPClient::onNegotiate]\n");
      return 0;
    }

    virtual void onHeartbeat(AMQP::TcpConnection* connection) override
    {
      printf("[AMQPClient::onHeartbeat]\n");
    }

    virtual void onConnected(AMQP::TcpConnection *connection) override
    {
      printf("[AMQPClient::onConnected]\n");
      if (auto_reconnect_) {
        StopReconnectTimer();
      }
      amqp_callbacks_->on_connected_cb(this);
    }

    virtual void onClosed(AMQP::TcpConnection* connection) override
    {
      printf("[AMQPClient::onClosed]\n");
      amqp_callbacks_->on_disconnected_cb(this);
      if (auto_reconnect_) {
        StartReconnectTimer();
      }
    }

    virtual void onError(AMQP::TcpConnection* connection, const char* errmsg) override
    {
      printf("[AMQPClient::onError] %s\n", errmsg);
      if (connection->fileno() < 0)
      {
        onClosed(connection);
      }
      else
      {
        amqp_callbacks_->on_error_cb(this, -1, errmsg);
      }
    }

    void onBindQueueSuccess()
    {
      printf("[AMQPClient::onBindQueueSuccess]\n");
    }

    void onBindQueueError(const char* errmsg)
    {
      printf("[AMQPClient::onBindQueueError] errmsg: %s\n", errmsg);
    }

    void onDeclareExchangeSuccess()
    {
      printf("[AMQPClient::onDeclareExchangeSuccess]\n");
    }

    void onDeclareExchangeError(const char* errmsg)
    {
      printf("[AMQPClient::onDeclareExchangeError] errmsg: %s\n", errmsg);
    }

    void onDeclareQueueSuccess(const std::string &queue_name, uint32_t msg_count, uint32_t consumer_count)
    {
      printf("[AMQPClient::onDeclareQueueSuccess] queue_name: %s, msg_count: %d, consumer_count: %d\n", queue_name.c_str(), msg_count, consumer_count);
    }

    void onConsumeSuccess(const std::string &tag)
    {
      printf("[AMQPClient::onConsumeSuccess] tag: %s\n", tag.c_str());
    }

    void onConsumeMessage(const AMQP::Message &message, uint64_t deliveryTag, bool redelivered)
    {
      printf("[AMQPClient::onConsumeMessage] deliveryTag: %ld\n", deliveryTag);
      AMQPMessage amqp_msg(&message);
      amqp_callbacks_->on_message_cb(this, &amqp_msg);
      channel_->ack(deliveryTag);
    }

  private:
    AMQP::Address         amqp_address_;
    AMQPTcpConnectionPtr  amqp_connection_;
    AMQPChannelPtr        channel_;
    AMQPCallbacksPtr      amqp_callbacks_;
    bool                  auto_reconnect_;
    PeriodicTimer         reconnect_timer_;
};

}

#endif  // _AMQP_CLIENT_H}
