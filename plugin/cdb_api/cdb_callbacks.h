#ifndef _CDB_CALLBACKS_H
#define _CDB_CALLBACKS_H

#include <stdio.h>
#include <functional>
#include <memory>

namespace cdb_api {

class CDBClient;
class CDBReply;
class CDBCommand;

typedef std::function<void (CDBClient*, const CDBReply*) >         OnReplyCallback;
typedef std::function<void (CDBClient*, const CDBCommand*) >       OnCmdSentCallback;
typedef std::function<void (CDBClient*) >                          OnConnectedCallback;
typedef std::function<void (CDBClient*) >                          OnClosedCallback;
typedef std::function<void (int, const char*) >                    OnErrorCallback;

struct CDBCallbacks {
    public:
    CDBCallbacks() :
        on_reply_cb(std::bind(&CDBCallbacks::EmptyReplyCb, this, std::placeholders::_1, std::placeholders::_2)),
        on_cmd_sent_cb(std::bind(&CDBCallbacks::EmptyCmdSentCb, this, std::placeholders::_1, std::placeholders::_2)),
        on_connected_cb(std::bind(&CDBCallbacks::EmptyConnectedCb, this, std::placeholders::_1)),
        on_closed_cb(std::bind(&CDBCallbacks::EmptyClosedCb, this, std::placeholders::_1)),
        on_error_cb(std::bind(&CDBCallbacks::EmptyErrorCb, this, std::placeholders::_1, std::placeholders::_2))
    { }

    public:
    OnReplyCallback     on_reply_cb;
    OnCmdSentCallback   on_cmd_sent_cb;
    OnConnectedCallback on_connected_cb;
    OnClosedCallback    on_closed_cb;
    OnErrorCallback     on_error_cb;

    private:
    void EmptyReplyCb(CDBClient*, const CDBReply*)             { printf("Empty CDBClient Reply Callback\n"); }
    void EmptyCmdSentCb(CDBClient*, const CDBCommand*)         { printf("Empty CDBClient Command Sent Callback\n"); }
    void EmptyConnectedCb(CDBClient*)                          { printf("Empty Connected Callback\n"); }
    void EmptyClosedCb(CDBClient*)                             { printf("Empty Connection Closed Callback\n"); }
    void EmptyErrorCb(int, const char*)                        { printf("Empty Connection Error Callback\n"); }
};

typedef std::shared_ptr<CDBCallbacks>       CDBCallbacksPtr;
typedef std::queue<OnReplyCallback>         CDBReplyCallbackQueue;

}  // namespace cdb_api

#endif  // _CDB_CALLBACKS_H
