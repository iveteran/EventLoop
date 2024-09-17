#include "tcp_callbacks.h"
#include "logger.h"

namespace evt_loop {

void TcpCallbacks::EmptyMsgRecvdCb(TcpConnection*, const Message*) {
    el_logger->debug("Empty Message Received Callback\n");
}
void TcpCallbacks::EmptyMsgSentCb(TcpConnection*, const Message*) {
    el_logger->debug("Empty Message Sent Callback\n");
}
void TcpCallbacks::EmptyClosedCb(TcpConnection*) {
    el_logger->debug("Empty Connection Closed Callback\n");
}
void TcpCallbacks::EmptyErrorCb(TcpConnection*, int, const char*) {
    el_logger->debug("Empty Connection Error Callback\n");
}
void TcpCallbacks::EmptyReadyCb(TcpConnection*) {
    el_logger->debug("Empty Connection Ready Callback\n");
}
void TcpCallbacks::EmptyIdleTimeoutCb(TcpConnection*, uint32_t) {
    el_logger->debug("Empty Connection Idle Timeout Callback\n");
}

}  // namespace evt_loop
