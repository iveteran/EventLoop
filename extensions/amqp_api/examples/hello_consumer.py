#!/bin/env python

import pika

def msg_consumer(channel, method, header, body):
  channel.basic_ack(delivery_tag=method.delivery_tag)
  if body == "quit":
    channel.basic_cancel(consumer_tag="hello-consumer")
    channel.stop_consuming()
  else:
    print body

def main():
  try:
    #credentials = pika.PlainCredentials("guest", "guest")
    credentials = pika.PlainCredentials("myuser", "mypwd")
    conn_params = pika.ConnectionParameters("localhost", virtual_host = '/mytest_vhost', credentials = credentials, ssl=True)

    conn_broker = pika.BlockingConnection(conn_params)
    channel = conn_broker.channel()
    channel.exchange_declare(exchange="hello-exchange",
        exchange_type="direct",
        passive=False,
        durable=True,
        auto_delete=False)

    channel.queue_declare(queue="hello-queue")
    channel.queue_bind(queue="hello-queue",
        exchange="hello-exchange",
        routing_key="hola")
    channel.basic_consume(msg_consumer,
        queue="hello-queue",
        consumer_tag="hello-consumer")

    channel.start_consuming()

  except Exception as e:
    print e

if __name__ == "__main__":
  main()

# NOTE: add rabbitmq user, permission and vhost before executing
#  1) robbitmyctl add_vhost /mytest_vhost
#  2) rabbitmqctl add_user myuser mypwd
#  3) rabbitmqctl set_permissions -p /mytest_vhost myuser '.*' '.*' '.*'
