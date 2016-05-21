#ifndef _SESSION_MNGR_H
#define _SESSION_MNGR_H

#include <memory>
#include "timer_handler.h"

namespace evt_loop {

typedef uint32_t    SessionID;

class TimeoutSession;
class TimeoutSessionManager;
typedef std::function<void (TimeoutSession*, uint32_t elapse)>  SessionFinishAction;

struct TimeoutSession
{
    friend TimeoutSessionManager;

    public:
    TimeoutSession() : m_id(0), m_ctime(0) {}

    protected:
    SessionID   m_id;
    time_t      m_ctime;
    SessionFinishAction m_finish_action;
    SessionFinishAction m_timeout_action;
};
typedef std::shared_ptr<TimeoutSession>  TimeoutSessionPtr;

class TimeoutSessionManager
{
    public:
    TimeoutSessionManager(uint32_t timeout = 0);
    void SetupTimeoutChecker(uint32_t timeout);
    void AddSession(const TimeoutSessionPtr& sess);
    TimeoutSession* GetSession(SessionID sid);
    void RemoveSession(SessionID cid);

    protected:
    void CheckSessionTimeoutCb(PeriodicTimer* timer);

    private:
    std::map<SessionID, TimeoutSessionPtr>  m_session_map;
    std::map<time_t, TimeoutSessionPtr>  m_sess_timeout_map;

    uint32_t m_timeout;
    PeriodicTimer m_timeout_checker;
};

}  // namespace evt_loop

#endif  // _SESSION_MNGR_H
