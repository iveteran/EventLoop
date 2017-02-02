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

      if (!m_activity_map.empty())
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
    if (m_client_map.find(conn->ID()) != m_client_map.end())
    {
        printf("[ConnectionManager::AddConnection] client (cid: %u) is exists, dosn't add again!\n", conn->ID());
        return;
    }
    ConnectionContextPtr conn_ctx = std::make_shared<ConnectionContext>(conn);
    m_client_map.insert(std::make_pair(conn->ID(), conn_ctx));
    m_activity_map.insert(std::make_pair(conn_ctx->act_time, conn_ctx));
    if (TimeoutCheckingEnabled() && !m_inactivity_checker.IsRunning())
        m_inactivity_checker.Start();
}
void ConnectionManager::ReplaceConnection(TcpConnection* conn)
{
    printf("[ConnectionManager::ReplaceConnection] new connection: { cid: %u, fd: %d }\n", conn->ID(), conn->FD());
    RemoveConnection(conn->ID(), true);
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
        if (close_connection)
            conn_ctx->conn->Disconnect();
        m_activity_map.erase(conn_ctx->act_time);
        m_client_map.erase(iter);
        printf("[ConnectionManager::RemoveConnection] cid: %u, fd: %d\n", conn_ctx->conn->ID(),conn_ctx->conn->FD());

        if (m_activity_map.empty() && m_inactivity_checker.IsRunning())
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
        m_activity_map.erase(act_time_old);
        m_activity_map.insert(std::make_pair(conn_ctx->act_time, conn_ctx));
    }
}
void ConnectionManager::OnConnectionInactivityCb(TimerEvent* timer)
{
    printf("[ConnectionManager::OnConnectionInactivityCb] Connections(%lu) inactivity checking on timer, now: %lu.\n", m_activity_map.size(), Now());
    for (auto iter = m_activity_map.begin(); iter != m_activity_map.end();)
    {
        time_t act_time = iter->first;
        TcpConnection* conn = iter->second->conn;
        time_t now = Now();
        printf("[ConnectionManager::OnConnectionInactivityCb] cid: %u, now: %lu, activity time: %lu\n", conn->ID(), now, act_time);
        uint32_t elapse = now - act_time;
        if (elapse >= m_timeout)
        {
            auto iter_rm = iter++;
            m_activity_map.erase(iter_rm);
            m_client_map.erase(conn->ID());

            //conn->Disconnect();
            m_conn_timeout_cb(conn, elapse);

            printf("[ConnectionManager::OnConnectionInactivityCb] Connection inactively in %u seconds\n", elapse);
        }
        else
        {
            break;  // stop traversing
        }
    }

    if (m_activity_map.empty())
        m_inactivity_checker.Stop();
}

}  // namespace evt_loop
