#ifndef _SESSION_MNGR_H
#define _SESSION_MNGR_H

#include "eventloop.h"
#include "tcp_connection.h"
#include "timer_handler.h"

namespace evt_loop {

typedef uint32_t    SessionID;

class Session;
typedef std::function<void (Session*)>  SessionFinishCallback;

struct Session
{
    Session() : create_time(0), requester(NULL), request(NULL), response(NULL) {}
    Session(TcpConnection* conn, Message* req) : create_time(Now()), requester(conn), request(req), response(NULL) { }

    SessionID sid;
    time_t create_time;
    SessionFinishCallback fin_action;

    TcpConnection* requester;
    Message* request;
    Message* response;
};
typedef std::shared_ptr<Session>  SessionPtr;

class SessionManager
{
    public:
    SessionManager(uint32_t timeout = 0);
    void SetupTimeoutChecker(uint32_t timeout);
    void AddSession(const SessionPtr& sess);
    void AddSession(TcpConnection* conn, Message* req);
    Session* GetSession(SessionID sid);
    void RemoveSession(SessionID cid);
    void OnSessionTimeout(Session* sess);
    void CheckSessionTimeoutCb(PeriodicTimer* timer);

    private:
    std::map<SessionID, SessionPtr>  m_session_map;
    std::map<time_t, SessionPtr>  m_sess_timeout_map;

    uint32_t m_timeout;
    PeriodicTimer m_timeout_checker;
};

}  // namespace evt_loop

#endif  // _SESSION_MNGR_H
