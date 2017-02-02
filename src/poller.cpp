#if defined(__linux__)
#include "poller_epoll.cpp"
#elif defined(__macosx__) || defined(__darwin__) || defined(__freebsd__)
#include "poller_kqueue.cpp"
#else
#error "platform unsupported"
#endif
