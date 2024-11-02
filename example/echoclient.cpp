#include <stdio.h>
#include "eventloop/el.h"

#define TEST_IP_V4
#define TEST_IP_V6
#define TEST_TIMER
//#define TEST_IDLE_TASK
//#define TEST_TICK_TASK

using namespace evt_loop;

class BusinessTester {
    public:
    BusinessTester() :
        dummy_(0)
#ifdef TEST_IP_V4
        , echoclient_(IPVer::V4, "localhost", 20000, MessageType::BINARY)
  #ifdef TEST_TIMER
        , sending_timer_(TimeVal(5, 0), std::bind(&BusinessTester::OnSendingTimer, this, std::placeholders::_1))
  #endif
#endif
#ifdef TEST_IP_V6
        , echoclient_ip6_(IPVer::V6, "::1", 30000, MessageType::CRLF)
  #ifdef TEST_TIMER
        , sending_timer_ip6_(TimeVal(10, 0), std::bind(&BusinessTester::OnSendingTimerIp6, this, std::placeholders::_1))
  #endif
#endif
    {
        TcpCallbacksPtr echo_client_cbs = std::shared_ptr<TcpCallbacks>(new TcpCallbacks);
        echo_client_cbs->on_msg_recvd_cb = std::bind(&BusinessTester::OnMessageRecvd, this, std::placeholders::_1, std::placeholders::_2);
        bool success;

#ifdef TEST_IP_V4
        echoclient_.SetNewClientCallback(std::bind(&BusinessTester::OnConnectionCreated, this, std::placeholders::_1));
        echoclient_.SetTcpCallbacks(echo_client_cbs);
        //echoclient_.EnableKeepAlive(true);
        echoclient_.EnableHeartbeat();

        success = echoclient_.Connect();
        assert(success);
        echoclient_.Send("hello world");
#endif

#ifdef TEST_IP_V6
        echoclient_ip6_.SetNewClientCallback(std::bind(&BusinessTester::OnConnectionCreated_ip6, this, std::placeholders::_1));
        TcpCallbacksPtr ip6_echo_client_cbs = std::shared_ptr<TcpCallbacks>(new TcpCallbacks);
        ip6_echo_client_cbs->on_msg_recvd_cb = std::bind(&BusinessTester::OnMessageRecvd_ip6, this, std::placeholders::_1, std::placeholders::_2);
        echoclient_ip6_.SetTcpCallbacks(ip6_echo_client_cbs);
        echoclient_ip6_.EnableKeepAlive(true);

        success = echoclient_ip6_.Connect();
        assert(success);
        echoclient_ip6_.Send("hello ipv6\r\n");
#endif
    }

    protected:
    void OnMessageRecvd(TcpConnection* conn, const Message* msg)
    {
        printf("[OnMessageRecvd] received message, fd: %d, message: %s, length: %lu\n", conn->FD(), msg->Payload(), msg->PayloadSize());
        if (!strncmp(msg->Payload(), "reconnect request", msg->PayloadSize()))
        {
            EV_Singleton->StopLoop();
        }
    }
    void OnConnectionCreated(TcpConnection* conn)
    {
        printf("[OnConnectionCreated] connection created, fd: %d\n", conn->FD());
        printf("[OnConnectionCreated] ping\n");
        conn->Send("ping");
#if defined(TEST_IP_V4) && defined(TEST_TIMER)
        sending_timer_.Start();
#endif
    }

    void OnConnectionCreated_ip6(TcpConnection* conn)
    {
        printf("[OnConnectionCreated_ip6] ip6 connection created, fd: %d\n", conn->FD());
        printf("[OnConnectionCreated_ip6] ping\n");
        conn->Send("ping ip6\r\n");
#if defined(TEST_IP_V6) && defined(TEST_TIMER)
        sending_timer_ip6_.Start();
#endif
    }
    void OnMessageRecvd_ip6(TcpConnection* conn, const Message* msg)
    {
        printf("[OnMessageRecvd_ip6] received message, fd: %d, message: %s, length: %lu\n", conn->FD(), msg->Payload(), msg->PayloadSize());
    }

#if defined(TEST_IP_V4) && defined(TEST_TIMER)
    void OnSendingTimer(TimerEvent* timer)
    {
        printf("[OnSendingTimer] ping\n");
        echoclient_.Send("ping");
    }
#endif

#if defined(TEST_IP_V6) && defined(TEST_TIMER)
    void OnSendingTimerIp6(TimerEvent* timer)
    {
        printf("[OnSendingTimerIp6] ping\n");
        echoclient_ip6_.Send("hello ipv6\r\n");
    }
#endif

    private:
    int dummy_;
#ifdef TEST_IP_V4
    TcpClient echoclient_;
  #ifdef TEST_TIMER
    PeriodicTimer sending_timer_;
  #endif
#endif

#ifdef TEST_IP_V6
    TcpClient echoclient_ip6_;
  #ifdef TEST_TIMER
    OneshotTimer  sending_timer_ip6_;
  #endif
#endif
};


int main(int argc, char **argv) {
  BusinessTester biz_tester;

  SignalHandler sh(SignalEvent::INT, [&](SignalHandler* sh, uint32_t signo) {
          printf("Shutdown\n");
          EV_Singleton->StopLoop();
          });

#ifdef TEST_IDLE_TASK
  IdleEvent idle_task([](UserEvent* idle_event, void* udata) {
          printf("[IdleEvent] Trigger idle event(id: %d), udata: %p\n", idle_event->Id(), udata);
          }, &biz_tester);
  IdleEvent idle_task2([](UserEvent* idle_event, void* udata) {
          printf("[IdleEvent] Trigger idle event(id: %d), udata: %p\n", idle_event->Id(), udata);
          }, NULL, 5);
#endif

#ifdef TEST_TICK_TASK
  TickEvent tick_task([](UserEvent* tick_event, void* udata) {
          printf("[TickEvent] Trigger tick event(id: %d), udata: %p\n", tick_event->Id(), udata);
          }, NULL, 10);
#endif

  EV_Singleton->StartLoop();

  return 0;
}

