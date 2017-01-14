#ifndef _AIO_WRAPPER_H
#define _AIO_WRAPPER_H

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <libaio.h>
#include <sys/eventfd.h>
#include <string>
#include <map>
#include <functional>
#include "fd_handler.h"

#include <sys/epoll.h>

using std::string;
using std::map;
using std::function;

namespace evt_loop {

typedef std::function<void (int status, const string& data)> aio_read_callback;
typedef std::function<void (int status)> aio_write_callback;
typedef std::function<void (int id)> request_done_callback;

class aio_wrapper;

class aio_request : public IOEvent
{
    friend aio_wrapper;

    public:
    enum { SUCCESS, FAILED, TIMEOUT };
    enum { AIO_MAXIO = 128, AIO_BLKSIZE = 4096 };

    public:
    aio_request(const char* filepath, int o_flags)
        : filepath_(filepath), fd_(-1), o_flags_(o_flags), efd_(-1), aio_ctx_(0), buffer_(NULL)
    {
        fd_ = open(filepath_.c_str(), o_flags_, 0644);
        iocb_ = (struct iocb*)malloc(sizeof(*iocb_));
    }
    virtual ~aio_request()
    {
        free(buffer_);
        free(iocb_);
    }
    bool init();
    bool submit();
    int get_fd() const { return fd_; }
    int get_eventfd() const { return efd_; }
    void get_aio_events_test() { get_aio_events(); }

    protected:
    virtual void perpare() = 0;
    virtual void on_aio_return(io_context_t ctx, struct iocb *iocb, long bytes, long status) = 0;

    void OnEvents(uint32_t events) override
    {
        //printf("[OnEvents]\n");
        if (events & IOEvent::READ) {
            get_aio_events();
        }
    }

    private:
    void get_aio_events();

    protected:
    string filepath_;
    int fd_;
    int o_flags_;
    int efd_;
    io_context_t aio_ctx_;
    struct iocb* iocb_;
    void* buffer_;
    request_done_callback done_cb_;
};
typedef std::shared_ptr<aio_request> aio_request_sp;

class aio_read_request : public aio_request
{
    public:
    aio_read_request(const char* filepath, int o_flags, const aio_read_callback& cb)
        : aio_request(filepath, o_flags), rd_offset_(0), completion_cb_(cb)
    {
    }

    protected:
    void perpare() override
    {
        io_prep_pread(iocb_, fd_, buffer_, AIO_BLKSIZE, rd_offset_);
    }
    void on_aio_return(io_context_t ctx, struct iocb *iocb, long rd_bytes, long status) override;

    private:
    string rx_buf_;
    size_t rd_offset_;
    aio_read_callback completion_cb_;
};
typedef std::shared_ptr<aio_read_request> aio_read_request_sp;

class aio_write_request : public aio_request
{
    public:
    aio_write_request(const char* filepath, int o_flags, const string& data, const aio_write_callback& cb)
        : aio_request(filepath, o_flags), tx_buf_(data), wt_offset_(0), completion_cb_(cb)
    {
    }

    protected:
    void perpare() override
    {
        io_prep_pwrite(iocb_, fd_, (void*)tx_buf_.data(), tx_buf_.size(), wt_offset_);
    }
    void on_aio_return(io_context_t ctx, struct iocb *iocb, long wt_bytes, long status) override;

    private:
    string tx_buf_;
    size_t wt_offset_;
    aio_write_callback completion_cb_;
};
typedef std::shared_ptr<aio_write_request> aio_write_request_sp;

class aio_wrapper
{
    public:
    static aio_wrapper& instance()
    {
        static aio_wrapper instance_;
        return instance_;
    }
    bool read(const char* filepath, int o_flags, const aio_read_callback& cb);
    bool write(const char* filepath, int o_flags, const string& data, const aio_write_callback& cb);

    void test();

    private:
    aio_wrapper() { };

    void release_request(int id)
    {
        //printf("[release_request]\n");
        req_map_.erase(id);
    }

    private:
    map<int, aio_request_sp> req_map_;
};
#define AIO     aio_wrapper::instance()

}  // ns evt_loop

#endif  // _AIO_WRAPPER_H
