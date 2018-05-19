#include "amqp_client.h"

namespace amqp_api
{

void AMQPClient::DeclareExchange(const char* exchange, AMQP::ExchangeType exchangeType, int flag)
{
  channel_->declareExchange(exchange, exchangeType, flag).onSuccess(
      std::bind(&AMQPClient::onDeclareExchangeSuccess, this)).onError(
      std::bind(&AMQPClient::onDeclareExchangeError, this, std::placeholders::_1));
}

void AMQPClient::DeclareQueue(const char* queue_name)
{
  channel_->declareQueue(queue_name).onSuccess(
      std::bind(&AMQPClient::onDeclareQueueSuccess, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)
      );
}

void AMQPClient::BindQueue(const char* exchange, const char* queue, const char* routingKey)
{
  channel_->bindQueue(exchange, queue, routingKey).onSuccess(
      std::bind(&AMQPClient::onBindQueueSuccess, this)).onError(
      std::bind(&AMQPClient::onBindQueueError, this, std::placeholders::_1));
}

void AMQPClient::Publish(const char* exchange, const char* routing_key, const char* message)
{
  channel_->publish(exchange, routing_key, message);//.onComplete();
}

void AMQPClient::Consume(const char* queue_name)
{
  channel_->consume(queue_name).onSuccess(
      std::bind(&AMQPClient::onConsumeSuccess, this, std::placeholders::_1)
      ).onMessage(
        std::bind(&AMQPClient::onConsumeMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
}

}  // namespace amqp_api
