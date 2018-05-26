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
    AMQPClient(const char* address, bool auto_reconnect = true);

    bool IsReady() { return amqp_connection_->fileno() > 0; }

    void SetCallbacks(const AMQPCallbacksPtr& amqp_cbs) { amqp_callbacks_ = amqp_cbs; }

    AMQPChannelPtr AMQPChannel() { return channel_; }

    void DeclareExchange(const char* exchange, AMQP::ExchangeType exchangeType, int flag);
    void DeclareQueue(const char* queue_name);
    void BindQueue(const char* exchange, const char* queue, const char* routingKey);
    void Publish(const char* exchange, const char* routing_key, const char* message);
    void Consume(const char* queue_name);

  private:
    void BuildChannel();
    void DestoryChannel();
    void Reconnect();
    void StartReconnectTimer();
    void StopReconnectTimer();
    void OnReconnectTimer(TimerEvent* timer);

  protected:
    virtual uint16_t onNegotiate(AMQP::TcpConnection *connection, uint16_t interval) override;
    virtual void onHeartbeat(AMQP::TcpConnection* connection) override;
    virtual void onConnected(AMQP::TcpConnection *connection) override;
    virtual void onClosed(AMQP::TcpConnection* connection) override;
    virtual void onError(AMQP::TcpConnection* connection, const char* errmsg) override;

    void onBindQueueSuccess();
    void onBindQueueError(const char* errmsg);
    void onDeclareExchangeSuccess();
    void onDeclareExchangeError(const char* errmsg);
    void onDeclareQueueSuccess(const std::string &queue_name, uint32_t msg_count, uint32_t consumer_count);
    void onConsumeSuccess(const std::string &tag);
    void onConsumeMessage(const AMQP::Message &message, uint64_t deliveryTag, bool redelivered);

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
