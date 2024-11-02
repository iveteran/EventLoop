#if defined(__linux__) && defined(USE_IO_URING)
#include <unistd.h>
#include <cstdio>
#include <sys/utsname.h>
#include <liburing.h>
#include "logger.h"
#include "poller_uring.h"
#include "uring_stream.h"
#include "uring_request.h"
#include "tcp_server.h"

#define MIN_KERNEL_VERSION      5
#define MIN_MAJOR_VERSION       5

namespace evt_loop {

Poller::Poller() {}
Poller::~Poller() {}
int Poller::SetEvents(int fd, PollerCtrl ctrl, uint32_t events, void* userdata) { return 0; }
int Poller::Poll(uint32_t wait_ms, const PollCallback& poll_cb) { return 0; }

static URingOptions _dft_uring_opt {
    //.sq_poll = true,
};

void fatal_error(const char *syscall) {
    perror(syscall);
    exit(1);
}

void make_sure_kernel_version_supported() {
    struct utsname buffer;
    char *p;
    long ver[16];
    int i=0;

    if (uname(&buffer) != 0) {
        perror("uname");
        exit(EXIT_FAILURE);
    }

    p = buffer.release;

    while (*p) {
        if (isdigit(*p)) {
            ver[i] = strtol(p, &p, 10);
            i++;
        } else {
            p++;
        }
    }
    el_logger->info("(io-uring) Minimum kernel version required is: {}.{}", MIN_KERNEL_VERSION, MIN_MAJOR_VERSION);
    if (ver[0] > MIN_KERNEL_VERSION || (ver[0] == MIN_KERNEL_VERSION && ver[1] >= MIN_MAJOR_VERSION) ) {
        el_logger->info("(io-uring) Your kernel version is: {}.{}, supports all features of IO URing(liburing)", ver[0], ver[1]);
        return;
    }

    fprintf(stderr, "(io-uring) Error: your kernel version is: %ld.%ld, does not support used features of io-ruing\n", ver[0], ver[1]);
    exit(EXIT_FAILURE);
}

int URingPoller::init_uring(const URingOptions& opt) {
    // initialize io_uring
    struct io_uring_params params;
    memset(&params, 0, sizeof(params));

    params.flags = IORING_SETUP_SINGLE_ISSUER;
	if (opt.defer_tw) {
		params.flags |= IORING_SETUP_DEFER_TASKRUN;
	} else if (opt.sq_poll) {
		params.flags = IORING_SETUP_SQPOLL;
		params.sq_thread_idle = 50;
	} else {
		params.flags |= IORING_SETUP_COOP_TASKRUN;
	}

    if (io_uring_queue_init_params(queue_depth_, ring_, &params) < 0) {
        perror("io_uring_init_failed...\n");
        exit(1);
    }

    // check if IORING_FEAT_FAST_POLL is supported
    if (!(params.features & IORING_FEAT_FAST_POLL)) {
        printf("IORING_FEAT_FAST_POLL not available in the kernel, quiting...\n");
        exit(0);
    }

    // check if buffer selection is supported
    struct io_uring_probe *probe;
    probe = io_uring_get_probe_ring(ring_);
    if (!probe || !io_uring_opcode_supported(probe, IORING_OP_PROVIDE_BUFFERS)) {
        printf("Buffer select not supported, skipping...\n");
        exit(0);
    }
    io_uring_free_probe(probe);

    return 0;
}

URingPoller::URingPoller(int queue_depth) : queue_depth_(queue_depth)
{
    make_sure_kernel_version_supported();
    el_logger->info("Poller: using io_uring on Linux platform");
    ring_ = new (struct io_uring);
    init_uring(_dft_uring_opt);
}

URingPoller::~URingPoller() {
    el_logger->info("Poller: exit io_uring");
    io_uring_queue_exit(ring_);
    delete ring_;
    ring_ = nullptr;
}

int URingPoller::Poll(uint32_t wait_ms, const PollCallback& poll_cb)
{
    //el_logger->debug("[URingPoller::Poll] io uring waiting, timeout: {}ms ...", wait_ms);

    struct __kernel_timespec ts;
    ts.tv_sec = wait_ms / 1000;
    ts.tv_nsec = (wait_ms - ts.tv_sec * 1000) * 1000000;

    struct io_uring_cqe *cqe;
    int ret = io_uring_submit_and_wait_timeout(ring_, &cqe, 1, &ts, NULL);
    if (ret == -ETIME) {
        return 0;  // timeout
    } else if (ret < 0) {
        fatal_error("io_uring_submit_and_wait exit");
    }

    uint32_t head = 0;
    int count = 0;
    io_uring_for_each_cqe(ring_, head, cqe) {
        ++count;

        auto req = (URingRequest*)cqe->user_data;
        int event_type = req->event_type;
        auto io_evt = req->io_evt;

        if (cqe->res == -ENOBUFS) {
            el_logger->critical("[URingPoller::Poll] bufs in automatic buffer selection empty, this should not happen...");
            abort();  // XXX: exit
        } else if (cqe->res < 0) {
            el_logger->error("[URingPoller::Poll] io_uring_for_each_cqe: cqe_res < 0, what: {} for event: {}, fd: {}",
                strerror(-cqe->res), event_type, io_evt->FD());
            continue;
        }

        switch (event_type) {
            case FileEvent::CREATE:
                {
                    int fd = cqe->res;
                    el_logger->debug("[URingPoller::Poll] new client, fd: {}", fd);
                    auto accept_req = (URingAcceptRequest*)req;
                    accept_req->client_fd = fd;
                    poll_cb(io_evt, event_type, req);
                    io_evt->UpdateEvents();  // re-add accepting request to SQ(submission queue)
                }
                break;
            case FileEvent::READ:
                {
                    uint32_t events = 0;
                    int rx_bytes = cqe->res;
                    el_logger->debug("[URingPoller::Poll] received bytes: {}", rx_bytes);
                    if (!rx_bytes) {
                        events |= FileEvent::CLOSED;
                    } else if (rx_bytes < 0) {
                        events |= FileEvent::ERROR;
                    } else {
                        events |= FileEvent::READ;
                        io_evt->UpdateEvents();  // re-add READ event
                    }
                    auto read_req = (URingReadRequest*)req;
                    read_req->rx_bytes = rx_bytes;
                    poll_cb(io_evt, events, req);
                }
                break;
            case FileEvent::WRITE:
                {
                    int tx_bytes = cqe->res;
                    el_logger->debug("[URingPoller::Poll] write done bytes: {}", tx_bytes);
                    auto write_req = (URingWriteRequest*)req;
                    write_req->tx_bytes = tx_bytes;
                    poll_cb(io_evt, event_type, req);
                }
                break;
            default:
                el_logger->error("Unknown event type: {}", event_type);
                break;
        }
        delete req;
    }
    io_uring_cq_advance(ring_, count);
    return count;
}

int URingPoller::SetEvents(int fd, PollerCtrl ctrl, uint32_t events, void* userdata)
{
    el_logger->debug("[URingPoller::SetEvents] fd: {}, ctrl: {}, events: {}", fd, (int)ctrl, events);
    auto io_evt = (IOEvent*)userdata;
    if (ctrl == PollerCtrl::ADD || ctrl == PollerCtrl::UPDATE) {
        if (events & FileEvent::CREATE) {
            auto server = static_cast<TcpServer*>(io_evt);
            AddAcceptRequest(fd, server);
        }
        if (events & FileEvent::READ) {
            auto stream = static_cast<URingStream*>(io_evt);
            AddReadRequest(fd, stream);
        }
        if (events & FileEvent::WRITE) {
            auto stream = static_cast<URingStream*>(io_evt);
            AddWriteRequest(fd, stream);
        }
    }
    return 0;
}

int URingPoller::AddAcceptRequest(int fd, TcpServer* server) {
    el_logger->debug("[URingPoller::AddAcceptRequest] fd: {}", fd);
    auto req = new URingAcceptRequest { {.event_type = FileEvent::CREATE, .io_evt = server } };
    socklen_t addr_len;
    struct sockaddr* peer_sockaddr;
    if (server->GetIPVersion() == IPVer::V4) {
        peer_sockaddr = (struct sockaddr*)&req->client_addr.addr;
        addr_len = sizeof(req->client_addr.addr);
    } else {
        peer_sockaddr = (struct sockaddr*)&req->client_addr.addr6;
        addr_len = sizeof(req->client_addr.addr6);
    }
    struct io_uring_sqe *sqe = io_uring_get_sqe(ring_);
    io_uring_prep_accept(sqe, fd, peer_sockaddr, &addr_len, 0);
    io_uring_sqe_set_data(sqe, req);
    io_uring_submit(ring_);

    return 0;
}

int URingPoller::AddReadRequest(int fd, URingStream* stream) {
    el_logger->debug("[URingPoller::AddReadRequest] fd: {}", fd);
    auto req = new URingReadRequest { { .event_type = FileEvent::READ, .io_evt = stream } };
    struct io_uring_sqe *sqe = io_uring_get_sqe(ring_);
    io_uring_prep_recv(sqe, fd, req->rx_buf, sizeof(req->rx_buf), 0);
    io_uring_sqe_set_data(sqe, req);
    io_uring_submit(ring_);
    el_logger->debug("[URingPoller::AddReadRequest] buf size: {}", sizeof(req->rx_buf));
    return 0;
}

int URingPoller::AddWriteRequest(int fd, URingStream* stream) {
    el_logger->debug("[URingPoller::AddWriteRequest] fd: {}", fd);
    auto req = new URingWriteRequest { { .event_type = FileEvent::WRITE, .io_evt = stream }, stream->tx_data_ };
    struct io_uring_sqe *sqe = io_uring_get_sqe(ring_);
    io_uring_prep_send(sqe, fd, req->tx_data.data(), req->tx_data.size(), 0);
    io_uring_sqe_set_data(sqe, req);
    io_uring_submit(ring_);
    el_logger->debug("[URingPoller::AddWriteRequest] data size: {}", req->tx_data.size());
    el_logger->debug("[URingPoller::AddWriteRequest] data: {}", req->tx_data);
    return 0;
}

}  // namespace evt_loop
#endif  // __linux__
