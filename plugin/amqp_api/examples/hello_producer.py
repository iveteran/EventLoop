#!/bin/env python

import pika, sys

def produce(msg):
  credentials = pika.PlainCredentials("guest", "guest")
  conn_params = pika.ConnectionParameters("localhost", credentials = credentials)

  conn_broker = pika.BlockingConnection(conn_params)

  channel = conn_broker.channel()

  channel.exchange_declare(exchange="hello-exchange", exchange_type="direct", passive=False, durable=True, auto_delete=False)

  msg_props = pika.BasicProperties()
  msg_props.content_type = "text/plain"

  channel.basic_publish(body=msg, exchange="hello-exchange", properties=msg_props, routing_key="hola")

def main():
  if len(sys.argv) != 2:
    print "%s MESSAGE" % sys.argv[0]
  else:
    msg = sys.argv[1]
    produce(msg)

if __name__ == "__main__":
  main()
