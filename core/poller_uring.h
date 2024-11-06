#ifndef _POLLER_URING_H
#define _POLLER_URING_H

#include "poller.h"

struct io_uring;

namespace evt_loop {

class TcpServer;
class URingStream;
class URingRequest;
class IPAddr;

struct URingOptions
{
    bool sq_poll;
    bool defer_tw;
    bool busy_loop;
    bool prefer_busy_poll;
    bool submit_all;
    int cp_entries = 0;
};

struct BufferContext {
	//struct msghdr msg;
	struct io_uring_buf_ring *buf_ring;
	char *buffer_base;
	int buf_shift;
	size_t buf_ring_size;
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

    int AddSendPacketRequest(IOEvent* io_evt, const IPAddr* remote_peer_addr, const string& data);
    int AddSendPacketRequest(IOEvent* io_evt, const IPAddr* remote_peer_addr,
            const char* data, size_t size, bool copy_mem=false);

    char* get_buffer(int idx);
    void recycle_buffer(int idx);

    protected:
    int AddAcceptRequest(int fd, TcpServer* server);
    int AddReadRequest(int fd, URingStream* stream);
    int AddWriteRequest(int fd, URingStream* stream);

    int RegisterFileHandler(IOEvent* io_evt);

    int AddRecvPacketRequest(IOEvent* io_evt);

    int OnReceivePacket(struct io_uring_cqe *cqe, URingRequest* req, const PollCallback& poll_cb);
    int OnSendPacketDone(struct io_uring_cqe *cqe, URingRequest* req, const PollCallback& poll_cb);

    struct io_uring_sqe* get_sqe();
    size_t buffer_size();
    int setup_buffer_pool();

    private:
    struct io_uring* ring_;
    int queue_depth_;

    struct BufferContext buf_ctx_;
};

}  // ns evt_loop

#endif  // _POLLER_URING_H
