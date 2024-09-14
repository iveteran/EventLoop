#ifndef _HEARTBEAT_MNGR_H
#define _HEARTBEAT_MNGR_H

#include <functional>
#include <memory>
#include "timer_handler.h"

using namespace std;

namespace evt_loop {

class TcpConnection;
class Message;
class TcpHeartbeatHandler;

struct HeartbeatParams
{
    uint32_t  idle_interval;
    uint32_t  ping_interval;
    uint32_t  ping_total;

    HeartbeatParams(uint32_t _1, uint32_t _2, uint32_t _3) :
        idle_interval(_1), ping_interval(_2), ping_total(_3)
    { }
};
using HeartbeatParamsPtr = std::shared_ptr<HeartbeatParams>;

using SendHeartbeatRequestCallback  = std::function<void (TcpConnection*)>;
using SendHeartbeatResponseCallback = std::function<void (TcpConnection*)>;
using HeartbeatTimeoutCallback = std::function<void (TcpConnection*)>;

class TcpHeartbeatHandler
{
    public:
    static const uint32_t DFT_IDLE_INTERVAL = 60;
    static const uint32_t DFT_PING_INTERVAL = 1;
    static const uint32_t DFT_PING_TOTAL = 5;

    private:
    class HeartbeatPing
    {
        public:
        HeartbeatPing(TcpHeartbeatHandler* hb_hdlr,
                uint32_t idle_interval = DFT_IDLE_INTERVAL,
                uint32_t ping_interval = DFT_PING_INTERVAL,
                uint32_t ping_total = DFT_PING_TOTAL);
        ~HeartbeatPing();

        void OnIdleTimer(TimerEvent* timer);
        void OnPingTimer(TimerEvent* timer);
        void OnFinishPing();

        private:
        TcpHeartbeatHandler*    hb_hdlr_;

        const uint32_t  m_idle_interval;
        const uint32_t  m_ping_interval;
        const uint32_t  m_ping_total;
        uint32_t        m_ping_count;

        PeriodicTimer   m_idle_timer;
        PeriodicTimer   m_ping_timer;
    };
    using HeartbeatPingPtr = std::shared_ptr<HeartbeatPing>;

    public:
    TcpHeartbeatHandler(TcpConnection* conn);

    void EnablePing(uint32_t idle_interval, uint32_t ping_interval, uint32_t ping_total);
    void DisablePing();

    void SetCallbacks(const SendHeartbeatRequestCallback& send_hb_req_cb = nullptr,
            const SendHeartbeatRequestCallback& send_hb_rsp_cb = nullptr,
            const HeartbeatTimeoutCallback& hb_timeout_cb = nullptr);

    bool IsHeartbeatRequest(const Message* msg);
    bool IsHeartbeatResponse(const Message* msg);

    void OnHeartbeatRequestReceived(const Message* msg);
    void OnHeartbeatResponseReceived(const Message* msg);
    void OnHeartbeatRequestSent(const Message* msg) { }
    void OnHeartbeatResponseSent(const Message* msg) { }

    private:
    bool IsBinaryHeartbeatRequest(const Message* msg);
    bool IsJsonHeartbeatRequest(const Message* msg);
    bool IsCRLFHeartbeatRequest(const Message* msg);

    bool IsBinaryHeartbeatResponse(const Message* msg);
    bool IsJsonHeartbeatResponse(const Message* msg);
    bool IsCRLFHeartbeatResponse(const Message* msg);

    void SendHeartbeatRequest(TcpConnection* conn);
    void SendBinaryHeartbeatRequest(TcpConnection* conn);
    void SendJsonHeartbeatRequest(TcpConnection* conn);
    void SendCRLFHeartbeatRequest(TcpConnection* conn);

    void SendHeartbeatResponse(TcpConnection* conn);
    void SendBinaryHeartbeatResponse(TcpConnection* conn);
    void SendJsonHeartbeatResponse(TcpConnection* conn);
    void SendCRLFHeartbeatResponse(TcpConnection* conn);

    void OnConnectionDead(TcpConnection* conn);

    private:
    TcpConnection*      m_connection;
    HeartbeatPingPtr    m_heartbeat_pinger;

    SendHeartbeatRequestCallback    m_send_heartbeat_request_cb;
    SendHeartbeatResponseCallback   m_send_heartbeat_response_cb;
    HeartbeatTimeoutCallback        m_heartbeat_timeout_cb;
};

using TcpHeartbeatHandlerPtr = std::shared_ptr<TcpHeartbeatHandler>;

}  // namespace evt_loop

#endif  // _HEARTBEAT_MNGR_H
