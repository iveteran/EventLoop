#include "session_mngr.h"

namespace evt_loop {

TimeoutSessionManager::TimeoutSessionManager(uint32_t timeout) :
    m_timeout(0),
    m_timeout_checker(std::bind(&TimeoutSessionManager::CheckSessionTimeoutCb, this, std::placeholders::_1))
{
    SetupTimeoutChecker(timeout);
}

void TimeoutSessionManager::SetupTimeoutChecker(uint32_t timeout)
{
  if (timeout != m_timeout) {
    if (timeout == 0) {
      m_timeout_checker.Stop();
      m_timeout = timeout;
    } else {
      if (m_timeout_checker.IsRunning()) {
        m_timeout_checker.Stop();
      }
      m_timeout = timeout;
      TimeVal tv(m_timeout, 0);
      m_timeout_checker.SetInterval(tv);
      m_timeout_checker.Start();
    }
  }
}
void TimeoutSessionManager::AddSession(const TimeoutSessionPtr& sess_ptr)
{
    m_session_map.insert(std::make_pair(sess_ptr->m_id, sess_ptr));
    m_sess_timeout_map.insert(std::make_pair(sess_ptr->m_ctime, sess_ptr));
}
TimeoutSession* TimeoutSessionManager::GetSession(SessionID sid)
{
    auto iter = m_session_map.find(sid);
    if (iter != m_session_map.end())
    {
        return iter->second.get();
    }
    return NULL;
}
void TimeoutSessionManager::RemoveSession(SessionID sid)
{
    auto iter = m_session_map.find(sid);
    if (iter != m_session_map.end())
    {
        auto& sess_ptr = iter->second;
        if (sess_ptr->m_finish_action)
        {
            uint32_t elapse = time(NULL) - sess_ptr->m_ctime;
            sess_ptr->m_finish_action(sess_ptr.get(), elapse);
        }
        m_sess_timeout_map.erase(sess_ptr->m_ctime);

        m_session_map.erase(iter);
    }
}
void TimeoutSessionManager::CheckSessionTimeoutCb(PeriodicTimer* timer)
{
    time_t now = time(NULL);
    printf("Session timeout checking on timer, now: %lu.\n", now);
    for (auto iter = m_sess_timeout_map.begin(); iter != m_sess_timeout_map.end();)
    {
        time_t ctime = iter->first;
        auto& sess_ptr = iter->second;
        uint32_t elapse = now - ctime;
        if (elapse >= m_timeout)
        {
            if (sess_ptr->m_timeout_action)
                sess_ptr->m_timeout_action(sess_ptr.get(), elapse);

            auto iter_rm = iter++;
            m_sess_timeout_map.erase(iter_rm);
            m_session_map.erase(sess_ptr->m_id);
            printf("[TimeoutSessionManager::CheckSessionTimeoutCb] Session timeout in %d seconds, dissconnect by server", elapse);
        }
        else
        {
            break;  // stop traversing
        }
    }
}

}  // namespace evt_loop
