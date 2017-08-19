#ifndef _CDB_CLIENT_H
#define _CDB_CLIENT_H

#include "el.h"
#include "utils.h"
#include "cdb_callbacks.h"

using namespace evt_loop;

namespace cdb_api {

static const bool RECONNECT = true;

class CDBReply
{
    public:
    virtual const void* GetReply() const = 0;
    virtual void SetReply(const void* reply) = 0;
};
struct CDBCommand
{
    char* format_str;
    std::vector<string> param_list;
};

class CDBClient
{
    public:
    CDBClient(bool auto_reconnect = true) :
        auto_reconnect_(auto_reconnect),
        reconnect_timer_(std::bind(&CDBClient::OnReconnectTimer, this, std::placeholders::_1))
    { }
    virtual ~CDBClient();

    virtual bool Init(const char* host, uint16_t port, const CDBCallbacksPtr& cdb_cbs = nullptr, bool auto_reconnect = true);

    bool Connect();
    virtual bool IsReady() const = 0;
    virtual void Disconnect() { };

    virtual bool SendCommand(CDBReply* reply_msg, const char* format, ...) = 0;
    virtual bool SendCommand(const OnReplyCallback& reply_cb, const char* format, ...) = 0;

    virtual bool HasError() const = 0;
    virtual const char* GetLastError() const = 0;

    protected:
    virtual bool Connect_(bool reconnect = false) = 0;
    void Reconnect();
    void OnReconnectTimer(TimerEvent* timer);

    protected:
    IPAddress           server_addr_;
    IPAddress           local_addr_;
    bool                auto_reconnect_;
    PeriodicTimer       reconnect_timer_;
    CDBCallbacksPtr     cdb_cbs_;
};

}  // namespace cdb_api

#endif  // _CDB_CLIENT_H
