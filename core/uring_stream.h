#ifndef _URING_IO_STREAM_H
#define _URING_IO_STREAM_H

#include "fd_handler.h"

namespace evt_loop
{

class URingStream : public BufferIOEvent {
    friend class URingPoller;

    public:
    URingStream(IOType io_type, int fd, uint32_t events = FileEvent::READ | FileEvent::ERROR)
        : BufferIOEvent(io_type, fd, events) {
        }
    ~URingStream() {
    }

    protected:
    void OnEvents(uint32_t events, void* ctx = nullptr) override;
    int OnRead(const void* buf, size_t bytes) override { return 0; }
    int OnWrite(const void* buf, size_t bytes) override { return 0; }

    void OnDataReceived(const char* data, size_t size);
    bool SendInner(const MessagePtr& msg) override;

    private:
    string tx_data_;  // temporary data for IO URing prepare to send
};

}  // namespace evt_loop

#endif  // _URING_IO_STREAM_H
