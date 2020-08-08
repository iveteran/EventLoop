#ifndef _DB_CALLBACKS_H
#define _DB_CALLBACKS_H

#include <stdio.h>
#include <functional>
#include <memory>
#include <queue>

namespace db_api {

class DBConnection;
class DBResult;
class SQLParameter;

typedef std::function<void (DBConnection*, const DBResult*) >         OnResultCallback;
typedef std::function<void (DBConnection*, const char* sql, const SQLParameter* sql_params) >     OnCmdSentCallback;
typedef std::function<void (DBConnection*) >                          OnConnectedCallback;
typedef std::function<void (DBConnection*) >                          OnClosedCallback;
typedef std::function<void (DBConnection*, int, const char*) >        OnErrorCallback;

struct DBCallbacks {
    public:
    DBCallbacks() :
        on_result_cb(std::bind(&DBCallbacks::EmptyReplyCb, this, std::placeholders::_1, std::placeholders::_2)),
        on_cmd_sent_cb(std::bind(&DBCallbacks::EmptyCmdSentCb, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)),
        on_connected_cb(std::bind(&DBCallbacks::EmptyConnectedCb, this, std::placeholders::_1)),
        on_closed_cb(std::bind(&DBCallbacks::EmptyClosedCb, this, std::placeholders::_1)),
        on_error_cb(std::bind(&DBCallbacks::EmptyErrorCb, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3))
    { }

    public:
    OnResultCallback    on_result_cb;
    OnCmdSentCallback   on_cmd_sent_cb;
    OnConnectedCallback on_connected_cb;
    OnClosedCallback    on_closed_cb;
    OnErrorCallback     on_error_cb;

    private:
    void EmptyReplyCb(DBConnection*, const DBResult*)             { printf("Empty DBConnection Result Callback\n"); }
    void EmptyCmdSentCb(DBConnection*, const char* sql, const SQLParameter*)         { printf("Empty DBConnection Command Sent Callback\n"); }
    void EmptyConnectedCb(DBConnection*)                          { printf("Empty Connected Callback\n"); }
    void EmptyClosedCb(DBConnection*)                             { printf("Empty Connection Closed Callback\n"); }
    void EmptyErrorCb(DBConnection*, int, const char*)            { printf("Empty Connection Error Callback\n"); }
};

typedef std::shared_ptr<DBCallbacks>       DBCallbacksPtr;
typedef std::queue<OnResultCallback>       DBReplyCallbackQueue;

}  // namespace db_api

#endif  // _DB_CALLBACKS_H
