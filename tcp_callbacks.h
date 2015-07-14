#ifndef _TCP_CALLBACKS_H
#define _TCP_CALLBACKS_H

#include "callback.h"

namespace richinfo {

class BusinessTester;
class TcpConnection;

typedef Callback2<BusinessTester, TcpConnection*, const string* > OnMsgRecvdCallback;
typedef Callback2<BusinessTester, TcpConnection*, const string* > OnMsgSentCallback;
typedef Callback<BusinessTester, TcpConnection* > OnNewClientCallback;
typedef Callback<BusinessTester, TcpConnection* > OnClosedCallback;
typedef Callback2<BusinessTester, int, const char* > OnErrorCallback;

struct TcpConnEventCallbacks {
    OnMsgRecvdCallback      on_msg_recvd_cb_;
    OnMsgSentCallback       on_msg_sent_cb_;
    OnClosedCallback        on_closed_cb_;
    OnErrorCallback         on_error_cb_;
};

struct TcpEventCallbacks {
    OnNewClientCallback     on_new_client_cb_;
    OnClosedCallback        on_closed_cb_;
    OnErrorCallback         on_error_cb_;
};

}  // namespace richinfo

#endif  // _TCP_CALLBACKS_H
