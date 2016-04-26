#include "connection_mngr.h"

#define CONNECTION_ACTIVITY_TIMEOUT (60 * 1)

namespace evt_loop {

ConnectionManager::ConnectionManager() :
    m_inactivity_checker(std::bind(&ConnectionManager::OnConnectionInactivityCb, this, std::placeholders::_1))
{
    TimeVal tv(CONNECTION_ACTIVITY_TIMEOUT, 0);
    m_inactivity_checker.SetInterval(tv);
    m_inactivity_checker.Start(EV_Singleton);
}

void ConnectionManager::AddConnection(TcpConnection* conn)
{
    ConnectionContextPtr conn_ctx = std::make_shared<ConnectionContext>(conn);
    m_client_map.insert(std::make_pair(conn->ID(), conn_ctx));
    m_activity_map.insert(std::make_pair(conn_ctx->act_time, conn_ctx));
}
TcpConnection* ConnectionManager::GetConnection(ClientID cid)
{
    auto iter = m_client_map.find(cid);
    if (iter != m_client_map.end())
    {
        return iter->second->conn;
    }
    return NULL;
}
void ConnectionManager::RemoveConnection(ClientID cid)
{
    auto iter = m_client_map.find(cid);
    if (iter != m_client_map.end())
    {
        auto& conn_ctx = iter->second;
        conn_ctx->conn->Disconnect();
        m_activity_map.erase(conn_ctx->act_time);

        m_client_map.erase(iter);
    }
}
void ConnectionManager::UpdateConnectionctivityTime(ClientID cid)
{
    auto iter = m_client_map.find(cid);
    if (iter != m_client_map.end())
    {
        auto& conn_ctx = iter->second;
        time_t act_time_old = conn_ctx->act_time;
        conn_ctx->act_time = Now();
        m_activity_map.erase(act_time_old);
        m_activity_map.insert(std::make_pair(conn_ctx->act_time, conn_ctx));
    }
}
void ConnectionManager::CloseInactivityConnection()
{
    for (auto iter = m_activity_map.begin(); iter != m_activity_map.end();)
    {
        time_t act_time = iter->first;
        auto& conn_ctx = iter->second;
        time_t now = Now();
        if (now - act_time >= CONNECTION_ACTIVITY_TIMEOUT)
        {
            conn_ctx->conn->Disconnect();

            auto iter_rm = iter++;
            m_activity_map.erase(iter_rm);
            m_client_map.erase(conn_ctx->conn->ID());
        }
        else
        {
            break;  // stop traversing
        }
    }
}

}  // namespace evt_loop
