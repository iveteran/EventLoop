#include "session_mngr.h"

#define SESSION_TIMEOUT (6 * 1)

namespace evt_loop {

SessionManager::SessionManager() :
    m_timeout_checker(std::bind(&SessionManager::CheckSessionTimeoutCb, this, std::placeholders::_1))
{
    TimeVal tv(SESSION_TIMEOUT, 0);
    m_timeout_checker.SetInterval(tv);
    m_timeout_checker.Start(EV_Singleton);
}

void SessionManager::AddSession(const SessionPtr& sess_ptr)
{
    m_session_map.insert(std::make_pair(sess_ptr->sid, sess_ptr));
    m_sess_timeout_map.insert(std::make_pair(sess_ptr->create_time, sess_ptr));
}
void SessionManager::AddSession(TcpConnection* conn, Message* msg)
{
    SessionPtr sess_ptr = std::make_shared<Session>(conn, msg);
    AddSession(sess_ptr);
}
Session* SessionManager::GetSession(SessionID sid)
{
    auto iter = m_session_map.find(sid);
    if (iter != m_session_map.end())
    {
        return iter->second.get();
    }
    return NULL;
}
void SessionManager::RemoveSession(SessionID sid)
{
    auto iter = m_session_map.find(sid);
    if (iter != m_session_map.end())
    {
        auto& sess_ptr = iter->second;
        sess_ptr->requester->Send(*sess_ptr->response);  // send timeout response
        m_sess_timeout_map.erase(sess_ptr->create_time);

        m_session_map.erase(iter);
    }
}
void SessionManager::CheckSessionTimeoutCb(PeriodicTimer* timer)
{
    printf("Session timeout checking on timer.\n");
    for (auto iter = m_sess_timeout_map.begin(); iter != m_sess_timeout_map.end();)
    {
        time_t create_time = iter->first;
        auto& sess_ptr = iter->second;
        time_t now = Now();
        if (now - create_time >= SESSION_TIMEOUT)
        {
            sess_ptr->requester->Send(*sess_ptr->response);  // send timeout response

            auto iter_rm = iter++;
            m_sess_timeout_map.erase(iter_rm);
            m_session_map.erase(sess_ptr->sid);
        }
        else
        {
            break;  // stop traversing
        }
    }
}

}  // namespace evt_loop
