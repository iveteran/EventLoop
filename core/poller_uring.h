#ifndef _POLLER_URING_H
#define _POLLER_URING_H

#include "poller.h"

struct io_uring;

namespace evt_loop {

class TcpServer;
class URingStream;

struct URingOptions
{
    bool sq_poll;
    bool defer_tw;
    bool busy_loop;
    bool prefer_busy_poll;
};

class URingPoller : public Poller
{
    public:
    static const int DFT_QUEUE_DEPTH = 256;

    public:
    URingPoller(int queue_depth = DFT_QUEUE_DEPTH);
    ~URingPoller();

    int init_uring(const URingOptions& opt);
    int SetEvents(int fd, PollerCtrl ctrl, uint32_t events, void* userdata) override;
    int Poll(uint32_t wait_ms, const PollCallback& poll_cb) override;

    protected:
    int AddAcceptRequest(int fd, TcpServer* server);
    int AddReadRequest(int fd, URingStream* stream);
    int AddWriteRequest(int fd, URingStream* stream);

    private:
    struct io_uring* ring_;
    int queue_depth_;
};

}  // ns evt_loop

#endif  // _POLLER_URING_H
