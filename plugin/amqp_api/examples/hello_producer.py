#!/bin/env python

import pika, sys

def produce(msg):
  try:
    #credentials = pika.PlainCredentials("guest", "guest")
    credentials = pika.PlainCredentials("myuser", "mypwd")
    conn_params = pika.ConnectionParameters("localhost", virtual_host = '/mytest_vhost', credentials = credentials)

    conn_broker = pika.BlockingConnection(conn_params)

    channel = conn_broker.channel()

    channel.exchange_declare(exchange="hello-exchange", exchange_type="direct", passive=False, durable=True, auto_delete=False)

    msg_props = pika.BasicProperties()
    msg_props.content_type = "text/plain"

    channel.basic_publish(body=msg, exchange="hello-exchange", properties=msg_props, routing_key="hola")
  except Exception as e:
    print e

def main():
  if len(sys.argv) != 2:
    print "%s MESSAGE" % sys.argv[0]
  else:
    msg = sys.argv[1]
    produce(msg)

if __name__ == "__main__":
  main()


# NOTE: add rabbitmq user, permission and vhost before executing
#  1) robbitmyctl add_vhost /mytest_vhost
#  2) rabbitmqctl add_user myuser mypwd
#  3) rabbitmqctl set_permissions -p /mytest_vhost myuser '.*' '.*' '.*'
