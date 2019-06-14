## EventLoop

EventLoop is an object oriented event loop for Linux/FreeBSD/MacOS in C++.

EventLoop是一个高并发、高性能的服务端网络库，采用C++语言实现，使用select/epoll/kqueue实现底层事件循环，
通过消息头的长度字段进行消息组包得到 完整的客户请求消息，通过c++11的functional回调返回客户请求消息给业务层，
支持用户态定时器、闲时任务调度、时间片调度，支持TLS、网络心跳、自动重连，
以扩展的方式支持异步的PostgreSQL client、Redis client、MQTT client、 RabbitMQ client、libcurl，及调用Lua脚本。
类似于libev/libuv，但封装上比它们更高层次，业务开发更方便简捷。
支持Linux、FreeBSD、MacOS、IOS。
