#include "connection_mngr.h"

namespace evt_loop {

ConnectionManager::ConnectionManager(uint32_t timeout) :
    m_timeout(0),
    m_inactivity_checker(std::bind(&ConnectionManager::OnConnectionInactivityCb, this, std::placeholders::_1))
{
    SetupInactivityChecker(timeout);
}

void ConnectionManager::SetupInactivityChecker(uint32_t timeout)
{
  if (timeout != m_timeout) {
    if (timeout == 0) {
      m_inactivity_checker.Stop();
      m_timeout = timeout;
    } else {
      if (m_inactivity_checker.IsRunning()) {
        m_inactivity_checker.Stop();
      }
      m_timeout = timeout;
      TimeVal tv(m_timeout, 0);
      m_inactivity_checker.SetInterval(tv);
      m_inactivity_checker.Start();
    }
  }
}
bool ConnectionManager::ConnectionExists(ClientID cid)
{
    return m_client_map.find(cid) != m_client_map.end();
}
void ConnectionManager::AddConnection(TcpConnection* conn)
{
    printf("[ConnectionManager::AddConnection] cid: %u\n", conn->ID());
    if (m_client_map.find(conn->ID()) != m_client_map.end())
    {
        printf("[ConnectionManager::AddConnection] client (cid: %u) is exists, dosn't add again!\n", conn->ID());
        return;
    }
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
    printf("[ConnectionManager::RemoveConnection] cid: %u\n", cid);
    auto iter = m_client_map.find(cid);
    if (iter != m_client_map.end())
    {
        auto& conn_ctx = iter->second;
        m_activity_map.erase(conn_ctx->act_time);

        m_client_map.erase(iter);
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
void ConnectionManager::CloseInactivityConnection()
{
    for (auto iter = m_activity_map.begin(); iter != m_activity_map.end();)
    {
        time_t act_time = iter->first;
        TcpConnection* conn = iter->second->conn;
        time_t now = Now();
        printf("[ConnectionManager::CloseInactivityConnection] cid: %u, now: %lu, activity time: %lu\n", conn->ID(), now, act_time);
        uint32_t elapse = now - act_time;
        if (elapse >= m_timeout)
        {
            auto iter_rm = iter++;
            m_activity_map.erase(iter_rm);
            m_client_map.erase(conn->ID());

            conn->Disconnect();

            printf("[ConnectionManager::CloseInactivityConnection] Connection inactively in %u seconds, disconnected by server\n", elapse);
        }
        else
        {
            break;  // stop traversing
        }
    }
}

}  // namespace evt_loop
