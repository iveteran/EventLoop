#ifndef _SESSION_MNGR_H
#define _SESSION_MNGR_H

#include "eventloop.h"
#include "tcp_connection.h"

namespace evt_loop {

typedef uint32_t    SessionID;

struct Session
{
    Session() : requester(NULL), request(NULL), response(NULL), create_time(0) {}
    Session(TcpConnection* conn, Message* req) : requester(conn), request(req), create_time(Now()) { }

    SessionID sid;
    TcpConnection* requester;
    Message* request;
    Message* response;
    time_t create_time;
};
typedef std::shared_ptr<Session>  SessionPtr;

class SessionManager
{
    public:
    SessionManager();
    void AddSession(const SessionPtr& sess);
    void AddSession(TcpConnection* conn, Message* req);
    Session* GetSession(SessionID sid);
    void RemoveSession(SessionID cid);
    void OnSessionTimeout(Session* sess);
    void CheckSessionTimeoutCb(PeriodicTimer* timer);

    private:
    std::map<SessionID, SessionPtr>  m_session_map;
    std::map<time_t, SessionPtr>  m_sess_timeout_map;

    PeriodicTimer m_timeout_checker;
};

}  // namespace evt_loop

#endif  // _SESSION_MNGR_H
