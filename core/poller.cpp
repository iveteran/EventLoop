#ifdef USE_SELECT
    #include "poller_select.cpp"
#else
    #if defined(__linux__)
        #if defined(USE_IO_URING)
            #include "poller_uring.cpp"
        #else
            #include "poller_epoll.cpp"
        #endif
    #elif defined(__OSX__) || defined(__DARWIN__) || defined(__APPLE__) || defined(__FREEBSD__)
        #include "poller_kqueue.cpp"
    #else
        #error "platform unsupported"
    #endif
#endif
