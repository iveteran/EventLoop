#include "aio_wrapper.h"
#include <sys/epoll.h>

namespace evt_loop {

/*
void aio_callback(io_context_t ctx, struct iocb *iocb, long res, long res2)
{
    printf("request_type: %s, data: %p, offset: %lld, length: %lu, buf: %p, buf str: %s, res: %ld, res2: %ld\n",
            (iocb->aio_lio_opcode == IO_CMD_PREAD) ? "READ" : "WRITE",
            iocb->data, iocb->u.c.offset, iocb->u.c.nbytes, iocb->u.c.buf, (char*)iocb->u.c.buf, res, res2);
}
*/

bool aio_request::init()
{
    int retval = io_setup(AIO_MAXIO, &aio_ctx_);
    if (retval != 0)
    {
        perror("io_setup failed");
        return false;
    }
    if (fd_ > 0) {
        int retval = posix_memalign(&buffer_, getpagesize(), AIO_BLKSIZE);
        if (retval < 0) {
            perror("posix_memalign failed");
            return false;
        }
        efd_ = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
        if (efd_ < 0) {
            perror("eventfd failed");
            return false;
        }
        perpare();
        io_set_eventfd(iocb_, efd_);
        //io_set_callback(iocb_, aio_callback);
        io_set_callback(iocb_, (io_callback_t)this);

        SetFD(efd_);
        return true;
    }
    return false;
}
bool aio_request::submit()
{
    int retval = io_submit(aio_ctx_, 1, (struct iocb**)(&iocb_));
    return retval >= 0;
}
void aio_request::get_aio_events()
{
    uint64_t done_num;
    if (read(efd_, &done_num, sizeof(done_num)) != sizeof(done_num)) {
        perror("read failed");
        return;
    }

    struct io_event events[AIO_MAXIO];
    while (done_num > 0) {
        struct timespec tms;
        tms.tv_sec = 0;
        tms.tv_nsec = 0;
        int r = io_getevents(aio_ctx_, 1, AIO_MAXIO, events, &tms);
        if (r > 0) {
            for (int i = 0; i < r; ++i) {
                aio_request* request = (aio_request*)(events[i].data);
                request->on_aio_return(aio_ctx_, events[i].obj, events[i].res, events[i].res2);
                //((io_callback_t)(events[i].data))(aio_ctx_, events[i].obj, events[i].res, events[i].res2);
            }
            done_num -= r;
        }
    }
}

void aio_read_request::on_aio_return(io_context_t ctx, struct iocb *iocb, long rd_bytes, long status)
{
    if ((size_t)rd_bytes < iocb->u.c.nbytes) {
        printf("[aio_read_request] request_type: %s, offset: %lld, to_read_size: %lu, buf: %p, rd_bytes: %ld, status: %ld\n",
                (iocb->aio_lio_opcode == IO_CMD_PREAD) ? "READ" : "WRITE",
                iocb->u.c.offset, iocb->u.c.nbytes, iocb->u.c.buf, rd_bytes, status);
    }

    if (rd_bytes > 0) {
        rx_buf_.append((char*)(iocb->u.c.buf), rd_bytes);
        rd_offset_ += rd_bytes;
        iocb->u.c.offset = rd_offset_;
    }
    if ((size_t)rd_bytes < iocb->u.c.nbytes) {
        if (completion_cb_) {
            completion_cb_(status, rx_buf_);
            done_cb_(efd_);
        }
    } else {
        submit();
    }
}

void aio_write_request::on_aio_return(io_context_t ctx, struct iocb *iocb, long wt_bytes, long status)
{
    /*
    printf("[aio_write_request] request_type: %s, offset: %lld, to_write_size: %lu, buf: %p, wt_bytes: %ld, status: %ld\n",
            (iocb->aio_lio_opcode == IO_CMD_PREAD) ? "READ" : "WRITE",
            iocb->u.c.offset, iocb->u.c.nbytes, iocb->u.c.buf, wt_bytes, status);
    */
    if (wt_bytes > 0) {
        wt_offset_ += wt_bytes;
        iocb->u.c.offset = wt_offset_;
    }
    if (wt_offset_ >= tx_buf_.size()) {
        printf("[aio_write_request] request_type: %s, offset: %lld, to_write_size: %lu, buf: %p, wt_bytes: %ld, status: %ld\n",
                (iocb->aio_lio_opcode == IO_CMD_PREAD) ? "READ" : "WRITE",
                iocb->u.c.offset, iocb->u.c.nbytes, iocb->u.c.buf, wt_bytes, status);
        if (completion_cb_) {
            completion_cb_(status);
            done_cb_(efd_);
        }
    } else {
        submit();
    }
}

bool aio_wrapper::read(const char* filepath, int o_flags, const aio_read_callback& cb)
{
    aio_read_request_sp sp = std::make_shared<aio_read_request>(filepath, o_flags, cb);
    bool success = sp->init() && sp->submit();
    if (success) {
        req_map_[sp->get_eventfd()] = sp;
        sp->done_cb_ = std::bind(&aio_wrapper::release_request, this, std::placeholders::_1);
        return true;
    } else {
        return false;
    }
}
bool aio_wrapper::write(const char* filepath, int o_flags, const string& data, const aio_write_callback& cb)
{
    aio_write_request_sp sp = std::make_shared<aio_write_request>(filepath, o_flags, data, cb);
    bool success = sp->init() && sp->submit();
    if (success) {
        req_map_[sp->get_eventfd()] = sp;
        sp->done_cb_ = std::bind(&aio_wrapper::release_request, this, std::placeholders::_1);
        return true;
    } else {
        return false;
    }
}
void aio_wrapper::test()
{
    int epfd = epoll_create(1);
    if (epfd == -1) {
        perror("epoll_create failed");
        return;
    }

    struct epoll_event epevent;
    epevent.events = EPOLLIN;

    for (auto& iter : req_map_) {
        printf("epoll_ctl add efd %d\n", iter.second->get_eventfd());
        epevent.data.ptr = iter.second.get();
        if (epoll_ctl(epfd, EPOLL_CTL_ADD, iter.second->get_eventfd(), &epevent)) {
            perror("epoll_ctl failed");
        }
    }

    while (true) {
        if (epoll_wait(epfd, &epevent, 1, -1) != 1) {
            perror("epoll_wait failed");
            return;
        }

        printf("epoll_wait epevent.data.ptr %p\n", epevent.data.ptr);
        aio_request* req = (aio_request*)epevent.data.ptr;
        req->get_aio_events_test();
    }
}

}  // ns evt_loop
