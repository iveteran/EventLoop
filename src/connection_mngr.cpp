#include "connection_mngr.h"
#include <functional>

namespace evt_loop {

static const uint32_t MIN_TIMEOUT = 30;
static const uint32_t MIN_TIME_SLICE = 5;

ConnectionManager::ConnectionManager(const ConnectionIdleTimeoutCallback& timeout_cb, uint32_t timeout) :
    m_timeout(0),
    m_conn_timeout_cb(timeout_cb),
    m_inactivity_checker(std::bind(&ConnectionManager::OnConnectionInactivityCb, this, std::placeholders::_1))
{
    uint32_t adj_timeout = (timeout == 0 ? 0 : std::max(timeout, MIN_TIMEOUT));
    SetupInactivityChecker(adj_timeout);
    printf("[ConnectionManager] timeout caller giving: %d, automated adjustment: %d\n", timeout, adj_timeout);
}

void ConnectionManager::SetupInactivityChecker(uint32_t timeout)
{
  if (timeout != m_timeout) {
    if (m_inactivity_checker.IsRunning()) {
      m_inactivity_checker.Stop();
    }

    if (timeout == 0) {
      m_timeout = timeout;
    } else {
      m_timeout = timeout;
      uint32_t time_slice = std::max(m_timeout / 20, MIN_TIME_SLICE);
      TimeVal tv(time_slice, 0);
      m_inactivity_checker.SetInterval(tv);

      if (!m_activity_map.Empty())
        m_inactivity_checker.Start();
    }
  }
}
CM_ENUM ConnectionManager::CheckConnectionExists(ClientID cid, TcpConnection* conn)
{
    auto iter = m_client_map.find(cid);
    if (iter != m_client_map.end())
    {
        if (iter->second->conn == conn)
            return CONNECTION_EXISTS_SAME;
        else
            return CONNECTION_EXISTS_ANOTHER;
    }
    else
        return CONNECTION_NOT_EXISTS;
}
void ConnectionManager::AddConnection(TcpConnection* conn)
{
    printf("[ConnectionManager::AddConnection] cid: %u, fd: %d\n", conn->ID(), conn->FD());
    CM_ENUM cm_status = CheckConnectionExists(conn->ID(), conn);
    if (cm_status == CONNECTION_EXISTS_SAME)
    {
        printf("[ConnectionManager::AddConnection] client (cid: %u, fd: %d) is exists, dosn't add again!\n", conn->ID(), conn->FD());
        return;
    }
    else if (cm_status == CONNECTION_EXISTS_ANOTHER)
    {
        printf("[ConnectionManager::AddConnection] replace the aged client for new client (cid: %u, fd: %d)\n", conn->ID(), conn->FD());
        const bool close_old = true;
        RemoveConnection(conn->ID(), close_old);
    }
    ConnectionContextPtr conn_ctx = std::make_shared<ConnectionContext>(conn);
    m_client_map.insert(std::make_pair(conn->ID(), conn_ctx));
    //m_activity_map.insert(std::make_pair(conn_ctx->act_time, conn_ctx));
    m_activity_map.Insert(conn_ctx->act_time, conn->ID(), conn_ctx);
    if (TimeoutCheckingEnabled() && !m_inactivity_checker.IsRunning())
        m_inactivity_checker.Start();
}
void ConnectionManager::ReplaceConnection(TcpConnection* conn, bool close_old)
{
    printf("[ConnectionManager::ReplaceConnection] new connection: { cid: %u, fd: %d }\n", conn->ID(), conn->FD());
    RemoveConnection(conn->ID(), close_old);
    AddConnection(conn);
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
void ConnectionManager::RemoveConnection(ClientID cid, bool close_connection)
{
    auto iter = m_client_map.find(cid);
    if (iter != m_client_map.end())
    {
        auto& conn_ctx = iter->second;
        printf("[ConnectionManager::RemoveConnection] cid: %u, fd: %d\n", conn_ctx->conn->ID(),conn_ctx->conn->FD());
        if (close_connection)
            conn_ctx->conn->Disconnect();
        //m_activity_map.erase(conn_ctx->act_time);
        m_activity_map.Erase(conn_ctx->act_time, cid);
        m_client_map.erase(iter);

        //if (m_activity_map.empty() && m_inactivity_checker.IsRunning())
        if (m_activity_map.Empty() && m_inactivity_checker.IsRunning())
            m_inactivity_checker.Stop();
    }
}
void ConnectionManager::UpdateConnectionctivityTime(ClientID cid)
{
    printf("[ConnectionManager::UpdateConnectionctivityTime] cid: %u, now: %lu\n", cid, Now());
    auto iter = m_client_map.find(cid);
    if (iter != m_client_map.end())
    {
        auto& conn_ctx = iter->second;
        time_t act_time_old = conn_ctx->act_time;
        conn_ctx->act_time = Now();
        //m_activity_map.erase(act_time_old);
        //m_activity_map.insert(std::make_pair(conn_ctx->act_time, conn_ctx));
        m_activity_map.Erase(act_time_old, cid);
        m_activity_map.Insert(conn_ctx->act_time, cid, conn_ctx);
    }
}
void ConnectionManager::OnConnectionInactivityCb(TimerEvent* timer)
{
    printf("[ConnectionManager::OnConnectionInactivityCb] Inactivity checking on timer, activity map size: %lu, activity element count: %lu, now: %lu\n",
            m_activity_map.Size(), m_activity_map.ElementCount(), Now());
    for (auto iter = m_activity_map.Begin(); iter != m_activity_map.End();)
    {
        time_t act_time = iter->first;
        time_t now = Now();
        uint32_t elapse = now - act_time;
        if (elapse >= m_timeout)
        {
            printf("[ConnectionManager::OnConnectionInactivityCb] Connection inactively in %u seconds, last activity time: %lu, now: %lu\n", elapse, act_time, now);
            auto& sub_map = iter->second;
            for (auto iter2 = sub_map.begin(); iter2 != sub_map.end(); ++iter2)
            {
                TcpConnection* conn = iter2->second->conn;
                m_client_map.erase(conn->ID());
                printf("[ConnectionManager::OnConnectionInactivityCb] Remove inactivity connection, conn id: %u\n", conn->ID());

                m_conn_timeout_cb(conn, elapse);
            }

            auto iter_rm = iter++;
            m_activity_map.Erase(iter_rm);
            printf("[ConnectionManager::OnConnectionInactivityCb] Remove inactivity connection, activity map size: %lu, activity element count: %lu, client map size: %lu\n",
                    m_activity_map.Size(), m_activity_map.ElementCount(), m_client_map.size());
        }
        else
        {
            break;  // stop traversing
        }
    }

    if (m_activity_map.Empty())
        m_inactivity_checker.Stop();
}

}  // namespace evt_loop
