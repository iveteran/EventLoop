#if defined(__linux__)
#include "poller_epoll.cpp"
#elif defined(__OSX__) || defined(__DARWIN__) || defined(__APPLE__) || defined(__FREEBSD__)
#include "poller_kqueue.cpp"
#else
#error "platform unsupported"
#endif
