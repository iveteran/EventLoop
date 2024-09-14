#ifndef _CONNECTION_MNGR_H
#define _CONNECTION_MNGR_H

#include "eventloop.h"
#include "tcp_connection.h"
#include "timer_handler.h"
#include "double_map.h"

namespace evt_loop {

typedef uint32_t    ClientID;
enum CM_ENUM {
    CONNECTION_NOT_EXISTS,
    CONNECTION_EXISTS_SAME,
    CONNECTION_EXISTS_ANOTHER,
};

struct ConnectionContext
{
    ConnectionContext() : conn(NULL), act_time(0) {}
    ConnectionContext(TcpConnection* c) : conn(c), act_time(Now()) { }

    TcpConnection* conn;
    time_t act_time;
};
typedef std::shared_ptr<ConnectionContext>  ConnectionContextPtr;
typedef std::function<void (TcpConnection*, uint32_t elapse)> ConnectionIdleTimeoutCallback;
typedef std::map<ClientID, ConnectionContextPtr>    CID_CONN_CTX_MAP;
typedef std::map<time_t, CID_CONN_CTX_MAP>          TIME_CONN_MAP_MAP;

class ConnectionManager
{
    public:
    ConnectionManager(const ConnectionIdleTimeoutCallback& timeout_cb, uint32_t timeout = 0);
    bool TimeoutCheckingEnabled() const { return m_timeout != 0; }
    void SetupInactivityChecker(uint32_t timeout);
    CM_ENUM CheckConnectionExists(ClientID cid, TcpConnection* conn);
    void AddConnection(TcpConnection* conn);
    void ReplaceConnection(TcpConnection* conn, bool close_old = false);
    TcpConnection* GetConnection(ClientID cid);
    void RemoveConnection(ClientID cid, bool close_connection = false);
    void UpdateConnectionctivityTime(ClientID cid);
    void OnConnectionInactivityCb(TimerEvent* timer);
    uint32_t ConnectionNumber() const { return m_client_map.size(); }

    private:
    CID_CONN_CTX_MAP    m_client_map;
    //TIME_CONN_MAP_MAP   m_activity_map;
    DoubleMap<time_t, ClientID, ConnectionContextPtr>   m_activity_map;

    uint32_t m_timeout;
    ConnectionIdleTimeoutCallback m_conn_timeout_cb;
    PeriodicTimer m_inactivity_checker;
};

}  // namespace evt_loop

#endif  // _CONNECTION_MNGR_H
