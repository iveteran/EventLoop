#include <stdio.h>
#include "el.h"

namespace evt_loop {

class BusinessTester {
    public:
    BusinessTester() :
        echoclient_("localhost", 20000, MessageType::BINARY),
        echoclient_ip6_("::1", 30000, MessageType::BINARY),
        sending_timer_(TimeVal(5, 0), std::bind(&BusinessTester::OnSendingTimer, this, std::placeholders::_1)),
        sending_timer_ip6_(TimeVal(10, 0), std::bind(&BusinessTester::OnSendingTimerIp6, this, std::placeholders::_1))
    {
        TcpCallbacksPtr echo_client_cbs = std::shared_ptr<TcpCallbacks>(new TcpCallbacks);
        echo_client_cbs->on_msg_recvd_cb = std::bind(&BusinessTester::OnMessageRecvd, this, std::placeholders::_1, std::placeholders::_2);

        echoclient_.SetNewClientCallback(std::bind(&BusinessTester::OnConnectionCreated, this, std::placeholders::_1));
        echoclient_.SetTcpCallbacks(echo_client_cbs);
        echoclient_.EnableKeepAlive(true);

        echoclient_.Connect();
        echoclient_.Send("hello world");

        echoclient_ip6_.SetNewClientCallback(std::bind(&BusinessTester::OnConnectionCreated_ip6, this, std::placeholders::_1));
        TcpCallbacksPtr ip6_echo_client_cbs = std::shared_ptr<TcpCallbacks>(new TcpCallbacks);
        ip6_echo_client_cbs->on_msg_recvd_cb = std::bind(&BusinessTester::OnMessageRecvd_ip6, this, std::placeholders::_1, std::placeholders::_2);
        echoclient_ip6_.SetTcpCallbacks(ip6_echo_client_cbs);
        echoclient_ip6_.EnableKeepAlive(true);

        echoclient_ip6_.Connect();
        echoclient_ip6_.Send("hello ipv6");
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
        sending_timer_.Start();
    }

    void OnConnectionCreated_ip6(TcpConnection* conn)
    {
        printf("[OnConnectionCreated_ip6] ip6 connection created, fd: %d\n", conn->FD());
        printf("[OnConnectionCreated_ip6] ping\n");
        conn->Send("ping ip6");
        sending_timer_ip6_.Start();
    }
    void OnMessageRecvd_ip6(TcpConnection* conn, const Message* msg)
    {
        printf("[OnMessageRecvd_ip6] received message, fd: %d, message: %s, length: %lu\n", conn->FD(), msg->Payload(), msg->PayloadSize());
    }

    void OnSendingTimer(TimerEvent* timer)
    {
        printf("[OnSendingTimer] ping\n");
        echoclient_.Send("ping");
    }

    void OnSendingTimerIp6(TimerEvent* timer)
    {
        printf("[OnSendingTimerIp6] ping\n");
        echoclient_ip6_.Send("hello ipv6");
    }

    private:
    TcpClient echoclient_;
    TcpClient6 echoclient_ip6_;
    PeriodicTimer sending_timer_;
    OneshotTimer  sending_timer_ip6_;
};
}   // ns evt_loop

using namespace evt_loop;

int main(int argc, char **argv) {
  BusinessTester biz_tester;

  SignalHandler sh(SignalEvent::INT, [&](SignalHandler* sh, uint32_t signo) {
          printf("Shutdown\n");
          EV_Singleton->StopLoop();
          });

  IdleEvent idle_task([](UserEvent* idle_event, void* udata) {
          printf("[IdleEvent] Trigger idle event(id: %d), udata: %p\n", idle_event->Id(), udata);
          }, &biz_tester);
  IdleEvent idle_task2([](UserEvent* idle_event, void* udata) {
          printf("[IdleEvent] Trigger idle event(id: %d), udata: %p\n", idle_event->Id(), udata);
          }, NULL, 5);

  TickEvent tick_task([](UserEvent* tick_event, void* udata) {
          printf("[TickEvent] Trigger tick event(id: %d), udata: %p\n", tick_event->Id(), udata);
          }, NULL, 10);

  EV_Singleton->StartLoop();

  return 0;
}

