#ifndef _URING_REQUEST_H
#define _URING_REQUEST_H

#include <netinet/in.h>

#define READ_BUF_SIZE 4096

namespace evt_loop
{

class IOEvent;

struct URingRequest {
    int event_type = 0;
    IOEvent* io_evt = nullptr;
};

struct URingAcceptRequest : public URingRequest {
    int client_fd = -1;
    union {
        struct sockaddr_in addr;
        struct sockaddr_in6 addr6;
    } client_addr;
};

struct URingReadRequest : public URingRequest {
    char rx_buf[READ_BUF_SIZE];
    int rx_bytes = 0;
};

struct URingWriteRequest : public URingRequest {
    string tx_data;
    int tx_bytes = 0;
};

struct URingRecvPacketRequest : public URingRequest {
    struct msghdr msg;
    struct iovec iov;
    int rx_bytes = 0;
    int buf_idx = 0;
};

struct URingSendPacketRequest : public URingRequest {
    struct msghdr msg;
    struct iovec iov;
    int tx_bytes = 0;
};

}  // evt_loop
#endif  // _URING_REQUEST_H
