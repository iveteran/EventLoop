#ifndef _TCP_CALLBACKS_H
#define _TCP_CALLBACKS_H

#include "callback.h"

namespace richinfo {

class TcpConnection;

template<typename T>
struct TcpCallbackType {
    typedef Callback2<T, TcpConnection*, const string* >    OnMsgRecvdCallback;
    typedef Callback2<T, TcpConnection*, const string* >    OnMsgSentCallback;
    typedef Callback<T, TcpConnection* >                    OnNewClientCallback;
    typedef Callback<T, TcpConnection* >                    OnClosedCallback;
    typedef Callback2<T, int, const char* >                 OnErrorCallback;
};

struct ITcpEventHandler {
    virtual void OnNewConnection(TcpConnection* conn) { } 
    virtual void OnMessageRecvd(TcpConnection* conn, const string* data) { }
    virtual void OnMessageSent(TcpConnection* conn, const string* data) { }
    virtual void OnConnectionClosed(TcpConnection* conn) { }
    virtual void OnError(int errcode, const char* errstr) { }
};

}  // namespace richinfo

#endif  // _TCP_CALLBACKS_H
