#ifndef _CONNECTION_MNGR_H
#define _CONNECTION_MNGR_H

#include "eventloop.h"
#include "tcp_connection.h"
#include "timer_handler.h"

namespace evt_loop {

typedef uint32_t    ClientID;

struct ConnectionContext
{
    ConnectionContext() : conn(NULL), act_time(0) {}
    ConnectionContext(TcpConnection* c) : conn(c), act_time(Now()) { }

    TcpConnection* conn;
    time_t act_time;
};
typedef std::shared_ptr<ConnectionContext>  ConnectionContextPtr;

class ConnectionManager
{
    public:
    ConnectionManager(uint32_t timeout = 0);
    void SetupInactivityChecker(uint32_t timeout);
    void AddConnection(TcpConnection* conn);
    TcpConnection* GetConnection(ClientID cid);
    void RemoveConnection(ClientID cid);
    void UpdateConnectionctivityTime(ClientID cid);
    void CloseInactivityConnection();
    void OnConnectionInactivityCb(PeriodicTimer* timer)
    {
        printf("Connections(%lu) inactivity checking on timer, now: %lu.\n", m_client_map.size(), Now());
        CloseInactivityConnection();
    }

    private:
    std::map<ClientID, ConnectionContextPtr>  m_client_map;
    std::map<time_t, ConnectionContextPtr>    m_activity_map;

    uint32_t m_timeout;
    PeriodicTimer m_inactivity_checker;
};

}  // namespace evt_loop

#endif  // _CONNECTION_MNGR_H
