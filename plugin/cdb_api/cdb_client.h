#ifndef _CDB_CLIENT_H
#define _CDB_CLIENT_H

#include "el.h"
#include "utils.h"
#include "cdb_callbacks.h"

using namespace evt_loop;

namespace cdb_api {

class CDBMessage
{
    public:
    virtual const void* GetReply() const = 0;
};

class CDBClient : public IOEvent
{
    public:
    CDBClient() : auto_reconnect_(true), reconnect_timer_(std::bind(&CDBClient::OnReconnectTimer, this, std::placeholders::_1)) { }
    virtual ~CDBClient();

    virtual bool Init(const char* host, uint16_t port, const CDBCallbacksPtr& cdb_cbs = nullptr, bool auto_reconnect = true);
    bool Connect();
    void SetCallbacks(const CDBCallbacksPtr& cdb_cbs);

    virtual bool IsReady() = 0;
    virtual void Disconnect() { };
    virtual bool SendCommand(const char* format, ...) = 0;
    virtual bool SendCommand(const OnReplyCallback& reply_cb, const char* format, ...) = 0;

    protected:
    virtual bool Connect_() = 0;
    void Reconnect();

    void OnReconnectTimer(PeriodicTimer* timer);

    protected:
    IPAddress           server_addr_;
    bool                auto_reconnect_;
    PeriodicTimer       reconnect_timer_;

    CDBCallbacksPtr         cdb_cbs_;
    CDBReplyCallbackQueue   reply_cb_queue_;
};

}  // namespace cdb_api

#endif  // _CDB_CLIENT_H
