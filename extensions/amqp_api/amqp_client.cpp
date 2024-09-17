#include <openssl/ssl.h>
#include "amqp_client.h"
#include "eventloop/logger.h"

namespace amqp_api
{

AMQPClient::AMQPClient(const char* address, bool auto_reconnect) :
  amqp_address_(address), auto_reconnect_(auto_reconnect),
  reconnect_timer_(std::bind(&AMQPClient::OnReconnectTimer, this, std::placeholders::_1))
{
  if (auto_reconnect_) {
    reconnect_timer_.SetInterval(TimeVal(1, 0));
  }

  if (strncmp(address, "amqps://", 8) == 0) {
    SSL_library_init();
  }

  BuildChannel();
}

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

void AMQPClient::BuildChannel()
{
  amqp_connection_ = std::make_shared<AMQP::TcpConnection>(this, amqp_address_);
  channel_ = std::make_shared<AMQP::TcpChannel>(amqp_connection_.get());
}

void AMQPClient::DestoryChannel()
{
  channel_ = nullptr;
  amqp_connection_ = nullptr;
}

void AMQPClient::Reconnect()
{
  DestoryChannel();
  BuildChannel();
}

void AMQPClient::StartReconnectTimer()
{
  if (!reconnect_timer_.IsRunning())
    reconnect_timer_.Start();
}

void AMQPClient::StopReconnectTimer()
{
  if (reconnect_timer_.IsRunning())
    reconnect_timer_.Stop();
}

void AMQPClient::OnReconnectTimer(TimerEvent* timer)
{
  el_logger->debug("[OnReconnectTimer begin] is ready: {}", IsReady());
  if (!IsReady()) {  // if the connection is not created, then reconnect
    Reconnect();
  } else {
    timer->Stop();
  }
}

uint16_t AMQPClient::onNegotiate(AMQP::TcpConnection *connection, uint16_t interval)
{
  el_logger->debug("[AMQPClient::onNegotiate]");
  return 0;
}

void AMQPClient::onHeartbeat(AMQP::TcpConnection* connection)
{
  el_logger->debug("[AMQPClient::onHeartbeat]");
}

void AMQPClient::onConnected(AMQP::TcpConnection *connection)
{
  el_logger->info("[AMQPClient::onConnected]");
  if (auto_reconnect_) {
    StopReconnectTimer();
  }
  amqp_callbacks_->on_connected_cb(this);
}

void AMQPClient::onClosed(AMQP::TcpConnection* connection)
{
  el_logger->info("[AMQPClient::onClosed]");
  amqp_callbacks_->on_disconnected_cb(this);
  if (auto_reconnect_) {
    StartReconnectTimer();
  }
}

void AMQPClient::onError(AMQP::TcpConnection* connection, const char* errmsg)
{
  el_logger->error("[AMQPClient::onError] {}", errmsg);
  if (connection->fileno() < 0)
  {
    onClosed(connection);
  }
  else
  {
    amqp_callbacks_->on_error_cb(this, -1, errmsg);
  }
}

void AMQPClient::onBindQueueSuccess()
{
  el_logger->debug("[AMQPClient::onBindQueueSuccess]");
}

void AMQPClient::onBindQueueError(const char* errmsg)
{
  el_logger->debug("[AMQPClient::onBindQueueError] errmsg: {}", errmsg);
}

void AMQPClient::onDeclareExchangeSuccess()
{
  el_logger->debug("[AMQPClient::onDeclareExchangeSuccess]");
}

void AMQPClient::onDeclareExchangeError(const char* errmsg)
{
  el_logger->error("[AMQPClient::onDeclareExchangeError] errmsg: {}", errmsg);
}

void AMQPClient::onDeclareQueueSuccess(const std::string &queue_name, uint32_t msg_count, uint32_t consumer_count)
{
  el_logger->debug("[AMQPClient::onDeclareQueueSuccess] queue_name: {}, msg_count: {}, consumer_count: {}",
          queue_name, msg_count, consumer_count);
}

void AMQPClient::onConsumeSuccess(const std::string &tag)
{
  el_logger->debug("[AMQPClient::onConsumeSuccess] tag: {}", tag);
}

void AMQPClient::onConsumeMessage(const AMQP::Message &message, uint64_t deliveryTag, bool redelivered)
{
  el_logger->debug("[AMQPClient::onConsumeMessage] deliveryTag: {}", deliveryTag);
  AMQPMessage amqp_msg(&message);
  amqp_callbacks_->on_message_cb(this, &amqp_msg);
  channel_->ack(deliveryTag);
}

}  // namespace amqp_api
