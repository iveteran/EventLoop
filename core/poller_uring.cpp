// Refers:
//   https://github.com/frevib/io_uring-echo-server.git
//   https://github.com/shuveb/loti-examples.git
//   https://github.com/axboe/liburing/examples/io_uring-udp.c
//
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

#include "udp_peer.h"
#include "ip_addr.h"

#define MIN_KERNEL_VERSION      5
#define MIN_MAJOR_VERSION       5

#include <sys/mman.h>
#define QD 64
#define BUF_SHIFT 12 /* 4k */
#define CQES (QD * 16)
#define BUFFERS CQES
#define CONTROLLEN 0

//#define SQE_FIXED_FD

namespace evt_loop {

Poller::Poller() {}
Poller::~Poller() {}
int Poller::SetEvents(int fd, PollerCtrl ctrl, uint32_t events, void* userdata) { return 0; }
int Poller::Poll(uint32_t wait_ms, const PollCallback& poll_cb) { return 0; }

static URingOptions _dft_uring_opt {
    //.sq_poll = true,
    .submit_all = true,
    .cp_entries = QD * 8,
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
	if (opt.submit_all) {
		params.flags |= IORING_SETUP_SUBMIT_ALL;
    }
    if (opt.cp_entries > 0) {
        params.cq_entries = opt.cp_entries;
        params.flags |= IORING_SETUP_CQSIZE;
    }

    if (io_uring_queue_init_params(queue_depth_, ring_, &params) < 0) {
        perror("io_uring_init_failed...\n");
        exit(1);
    }

    if (setup_buffer_pool() < 0) {
        fprintf(stderr, "setup_buffer_pool failed, quiting...\n");
        exit(0);
    }

    // check if IORING_FEAT_FAST_POLL is supported
    if (!(params.features & IORING_FEAT_FAST_POLL)) {
        fprintf(stderr, "IORING_FEAT_FAST_POLL not available in the kernel, quiting...\n");
        exit(0);
    }

    // check if buffer selection is supported
    struct io_uring_probe *probe;
    probe = io_uring_get_probe_ring(ring_);
    if (!probe || !io_uring_opcode_supported(probe, IORING_OP_PROVIDE_BUFFERS)) {
        fprintf(stderr, "Buffer select not supported, skipping...\n");
        exit(0);
    }
    io_uring_free_probe(probe);

    return 0;
}

size_t URingPoller::buffer_size()
{
    return 1U << buf_ctx_.buf_shift;
}

char* URingPoller::get_buffer(int idx)
{
	return buf_ctx_.buffer_base + (idx << buf_ctx_.buf_shift);
}

void URingPoller::recycle_buffer(int idx)
{
    io_uring_buf_ring_add(buf_ctx_.buf_ring, get_buffer(idx), buffer_size(), idx,
                  io_uring_buf_ring_mask(BUFFERS), 0);
    io_uring_buf_ring_advance(buf_ctx_.buf_ring, 1);
}

int URingPoller::setup_buffer_pool()
{
    size_t buf_size = buffer_size();
    el_logger->debug("[URingPoller::setup_buffer_pool]");
	buf_ctx_.buf_ring_size = (sizeof(struct io_uring_buf) + buf_size) * BUFFERS;

	void* mapped = mmap(NULL, buf_ctx_.buf_ring_size, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, 0, 0);
	if (mapped == MAP_FAILED) {
		el_logger->error("[URingPoller::setup_buffer_pool] mmap failed: {}", strerror(errno));
		return -1;
	}
	buf_ctx_.buf_ring = (struct io_uring_buf_ring *)mapped;

	io_uring_buf_ring_init(buf_ctx_.buf_ring);

	struct io_uring_buf_reg reg {
		.ring_addr = (unsigned long)buf_ctx_.buf_ring,
		.ring_entries = BUFFERS,
		.bgid = 0
	};
	buf_ctx_.buffer_base = (char *)buf_ctx_.buf_ring + sizeof(struct io_uring_buf) * BUFFERS;

	int ret = io_uring_register_buf_ring(ring_, &reg, 0);
	if (ret) {
		el_logger->error("buf_ring init failed: {} NB This requires a kernel version >= 6.0", strerror(-ret));
		return ret;
	}

	for (int i = 0; i < BUFFERS; i++) {
		io_uring_buf_ring_add(buf_ctx_.buf_ring, get_buffer(i), buf_size, i, io_uring_buf_ring_mask(BUFFERS), i);
	}
	io_uring_buf_ring_advance(buf_ctx_.buf_ring, BUFFERS);

	return 0;
}

struct io_uring_sqe* URingPoller::get_sqe() {
    struct io_uring_sqe* sqe = io_uring_get_sqe(ring_);
    if (!sqe) {
        io_uring_submit(ring_);
        sqe = io_uring_get_sqe(ring_);
    }
    return sqe;
}

URingPoller::URingPoller(int queue_depth) : queue_depth_(queue_depth)
{
    make_sure_kernel_version_supported();
    el_logger->info("Poller: using io_uring on Linux platform");
    ring_ = new (struct io_uring);

    memset(&buf_ctx_, 0, sizeof(buf_ctx_));
    buf_ctx_.buf_shift = BUF_SHIFT;

    init_uring(_dft_uring_opt);
}

URingPoller::~URingPoller() {
    el_logger->info("Poller: exit io_uring");
    io_uring_queue_exit(ring_);
    delete ring_;
    ring_ = nullptr;

    munmap(buf_ctx_.buf_ring, buf_ctx_.buf_ring_size);
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

        bool dont_delete_req = false;
        auto req = (URingRequest*)cqe->user_data;
        int event_type = req->event_type;
        auto io_evt = req->io_evt;

        if (cqe->res == -ENOBUFS) {
            el_logger->critical("[URingPoller::Poll] bufs in automatic buffer selection empty, this should not happen...");
            abort();  // XXX: exit
        } else if (cqe->res < 0) {
            el_logger->error("[URingPoller::Poll] io_uring_for_each_cqe: cqe_res < 0, what: {} for event type: {}, fd: {}",
                strerror(-cqe->res), event_type, io_evt->FD());
            continue;
        }

        switch (event_type) {
            case FileEvent::CREATE:
                {
                    int fd = cqe->res;
                    el_logger->debug("[URingPoller::Poll] new client, fd: {}", fd);
                    if (io_evt->GetIOType() == IOEvent::IOType::TCP_SERVER) {
                        auto accept_req = (URingAcceptRequest*)req;
                        accept_req->client_fd = fd;
                        poll_cb(io_evt, event_type, req);
                        io_evt->UpdateEvents();  // re-add accepting request to SQ(submission queue)
                    } else if (io_evt->GetIOType() == IOEvent::IOType::UDP_PEER) {
                        RegisterFileHandler(io_evt);
                    }
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
            case FileEvent::RCVPKT:
                OnReceivePacket(cqe, req, poll_cb);
                dont_delete_req = true;  // FIXME: will occurs double free error for deleting URingRecvPacketRequest, temporary solution
                break;
            case FileEvent::SNDPKT:
                OnSendPacketDone(cqe, req, poll_cb);
                break;
            default:
                el_logger->error("Unknown event type: {}", event_type);
                break;
        }
        if (! dont_delete_req) {
            delete req;  // FIXME: will occurs double free error for deleting URingRecvPacketRequest, temporarily use control variable dont_delete_req
        }
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
            if (io_evt->GetIOType() == IOEvent::IOType::TCP_SERVER) {
                auto server = static_cast<TcpServer*>(io_evt);
                AddAcceptRequest(fd, server);
            } else if (io_evt->GetIOType() == IOEvent::IOType::UDP_PEER) {
                RegisterFileHandler(io_evt);
            }
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

int URingPoller::RegisterFileHandler(IOEvent* io_evt) {
    el_logger->debug("[URingPoller::RegisterFileHandler] fd: {}", io_evt->FD());
    int fd = io_evt->FD();
    int ret = io_uring_register_files(ring_, &fd, 1);
    if (ret < 0) {
        el_logger->error("[URingPoller::SetEvents] Register fd {} failed: {}", fd, strerror(-ret));
        return -1;
    }
    ret = AddRecvPacketRequest(io_evt);
    if (ret < 0) {
        return ret;
    }

    return 0;
}

int URingPoller::AddRecvPacketRequest(IOEvent* io_evt) {
    el_logger->debug("[URingPoller::AddRecvPacketRequest] fd: {}", io_evt->FD());
    struct io_uring_sqe* sqe = get_sqe();
    if (!sqe) {
        el_logger->error("[URingPoller::AddRecvPacketRequest] cannot get sqe");
        return -1;
    }

    auto req = new URingRecvPacketRequest { { .event_type = FileEvent::RCVPKT, .io_evt = io_evt } };
    req->msg = {};
    req->msg.msg_namelen = sizeof(struct sockaddr_storage);
    req->msg.msg_controllen = CONTROLLEN;

#ifdef SQE_FIXED_FD
    int fd_idx = 0; // FIXME: it's not fixed to zero
    io_uring_prep_recvmsg_multishot(sqe, fd_idx, &req->msg, MSG_TRUNC);
    sqe->flags |= IOSQE_FIXED_FILE;
#else
    io_uring_prep_recvmsg_multishot(sqe, io_evt->FD(), &req->msg, MSG_TRUNC);
#endif
    sqe->flags |= IOSQE_BUFFER_SELECT;
    sqe->buf_group = 0;

    io_uring_sqe_set_data(sqe, req);

    return 0;
}

int URingPoller::AddSendPacketRequest(IOEvent* io_evt, const IPAddr* remote_peer_addr, const string& data) {
    bool copy_mem = true;
    return AddSendPacketRequest(io_evt, remote_peer_addr, data.data(), data.size(), copy_mem);
}

int URingPoller::AddSendPacketRequest(IOEvent* io_evt, const IPAddr* remote_peer_addr,
        const char* data, size_t data_size, bool copy_mem) {
    el_logger->debug("[URingPoller::AddSendPacketRequest] fd: {}, data size: {}", io_evt->FD(), data_size);

    struct io_uring_sqe *sqe = get_sqe();
    if (!sqe) {
        el_logger->error("[URingPoller::AddSendPacketRequest] cannot get sqe");
        return -1;
    }
    auto req = new URingSendPacketRequest { { .event_type = FileEvent::SNDPKT, .io_evt = io_evt } };
    io_uring_sqe_set_data(sqe, req);

    const char* tx_buf;
    if (copy_mem) {
        int buf_idx = 0;  // FIXME: MUST use unused buffer index, not fixed the zero index
        tx_buf = (char*)get_buffer(buf_idx);
        memcpy((void*)tx_buf, data, data_size);
    } else {
        tx_buf = data;
    }

    req->iov = (struct iovec) {
        .iov_base = (void*)tx_buf,
        .iov_len = data_size
    };
    req->msg = (struct msghdr) {
        .msg_name = (void*)remote_peer_addr->SockAddr(),
        .msg_namelen = (socklen_t)remote_peer_addr->Size(),
        .msg_iov = &req->iov,
        .msg_iovlen = 1,
        .msg_control = NULL,
        .msg_controllen = CONTROLLEN,
    };

#ifdef SQE_FIXED_FD
    int fd_idx = 0; // FIXME: it's not fixed to zero
    io_uring_prep_sendmsg(sqe, fd_idx, &req->msg, 0);
    sqe->flags |= IOSQE_FIXED_FILE;
#else
    io_uring_prep_sendmsg(sqe, io_evt->FD(), &req->msg, 0);
#endif
    return data_size;
}

int URingPoller::OnReceivePacket(struct io_uring_cqe *cqe, URingRequest* req, const PollCallback& poll_cb) {
    el_logger->debug("[URingPoller::OnReceivePacket] fd: {}", req->io_evt->FD());
    uint32_t events = 0;
    events |= FileEvent::READ;

    if (!(cqe->flags & IORING_CQE_F_MORE)) {
        int ret = AddRecvPacketRequest(req->io_evt);
        if (ret < 0) {
            return ret;
        }
    }

    if (cqe->res == -ENOBUFS)
        return 0;

    if (!(cqe->flags & IORING_CQE_F_BUFFER) || cqe->res < 0) {
        el_logger->error("[URingPoller::Poll] recv cqe bad result: {}", cqe->res);
        if (cqe->res == -EFAULT || cqe->res == -EINVAL)
            el_logger->error("NB: This requires a kernel version >= 6.0");
        return -1;
    }

    int rx_bytes = cqe->res;
    auto rx_req = (URingRecvPacketRequest*)req;
    rx_req->rx_bytes = rx_bytes;
    rx_req->buf_idx = cqe->flags >> 16; // XXX: why use the value of cqe->flags right shift 16 bits

    //ReceivePacket(req);
    poll_cb(req->io_evt, events, rx_req);

    return rx_bytes;
}

int URingPoller::OnSendPacketDone(struct io_uring_cqe *cqe, URingRequest* req, const PollCallback& poll_cb) {
    el_logger->debug("[URingPoller::OnSendPacketDone] fd: {}", req->io_evt->FD());
    uint32_t events = 0;
    events |= FileEvent::WRITE_DONE;
    if (cqe->res < 0) {
        el_logger->error("[URingPoller::Poll] error: {}", strerror(-cqe->res));
        events |= FileEvent::ERROR;
    }
    int tx_bytes = cqe->res;
    /*
    auto tx_req = (URingSendPacketRequest*)req;
    tx_req->tx_bytes = tx_bytes;
    */
    poll_cb(req->io_evt, events, &tx_bytes);

    return tx_bytes;
}

}  // namespace evt_loop
#endif  // __linux__
