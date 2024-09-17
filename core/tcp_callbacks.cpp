#include "tcp_callbacks.h"
#include "logger.h"

namespace evt_loop {

void TcpCallbacks::EmptyMsgRecvdCb(TcpConnection*, const Message*) {
    el_logger->debug("Empty Message Received Callback");
}
void TcpCallbacks::EmptyMsgSentCb(TcpConnection*, const Message*) {
    el_logger->debug("Empty Message Sent Callback");
}
void TcpCallbacks::EmptyClosedCb(TcpConnection*) {
    el_logger->debug("Empty Connection Closed Callback");
}
void TcpCallbacks::EmptyErrorCb(TcpConnection*, int, const char*) {
    el_logger->debug("Empty Connection Error Callback");
}
void TcpCallbacks::EmptyReadyCb(TcpConnection*) {
    el_logger->debug("Empty Connection Ready Callback");
}
void TcpCallbacks::EmptyIdleTimeoutCb(TcpConnection*, uint32_t) {
    el_logger->debug("Empty Connection Idle Timeout Callback");
}

}  // namespace evt_loop
