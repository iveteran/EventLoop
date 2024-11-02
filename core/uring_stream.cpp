#include <errno.h>
#include "eventloop.h"
#include "uring_stream.h"
#include "uring_request.h"

namespace evt_loop
{

void URingStream::OnEvents(uint32_t events, void* ctx) {
    if (events & FileEvent::WRITE_DONE) {
        auto req = (URingWriteRequest*)ctx;
        OnWriteDone(req->tx_bytes);
    }
    if (events & FileEvent::READ) {
        auto req = (URingReadRequest*)ctx;
        auto rx_bytes = req->rx_bytes;
        auto rx_buf = req->rx_buf;
        printf("--------> URingStream::OnEvents, rx_bytes: %d\n", rx_bytes);
        printf("--------> URingStream::OnEvents, data: %s\n", string(rx_buf, rx_bytes).c_str());
        int hs_status = 0;
        if (state_ == CONNECTED || state_ == HANDSHAKING) {
            hs_status = OnHandshake(rx_buf, rx_bytes);
            if (hs_status < 0) events |= FileEvent::CLOSED;
        }
        // if handshaking returns 0 means does not handshake, continue to process received data
        if (hs_status == 0) {
            OnDataReceived(rx_buf, rx_bytes);
        }
    }

    if (events & FileEvent::CLOSED) {
        OnClosed();
    } else if ((events & FileEvent::ERROR)) {
        OnError(errno, strerror(errno));
    }
}

void URingStream::OnDataReceived(const char* data, size_t size) {
    if (size == 0) {
        return;
    }

    rx_msg_mq_.FeedData(data, size);
    UpdateRxStats((uint32_t)size);

    if (rx_msg_mq_.HasCompletion()) {
        HandleCompletionMessages();
    }
}

bool URingStream::SendInner(const MessagePtr& msg) {
    if (FD() < 0) return false;

    tx_data_ = msg->Data();
    el_->GetPoller()->SetEvents(FD(), PollerCtrl::ADD, FileEvent::WRITE, this);

    return true;
}

}  // namespace evt_loop
