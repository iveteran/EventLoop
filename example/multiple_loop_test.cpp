#include <stdio.h>
#include <thread>

#include "el.h"

namespace evt_loop {

class ClientThreadTest {
    public:
    ClientThreadTest() :
      echoclient_binary_("localhost", 10000, MessageType::BINARY)
    {
        TcpCallbacksPtr echo_client_cbs = std::shared_ptr<TcpCallbacks>(new TcpCallbacks);
        echo_client_cbs->on_msg_recvd_cb = std::bind(&ClientThreadTest::OnMessageRecvd_Client, this, std::placeholders::_1, std::placeholders::_2);
        echo_client_cbs->on_conn_ready_cb = std::bind(&ClientThreadTest::OnClientConnectionReady, this, std::placeholders    ::_1);

        echoclient_binary_.SetTcpCallbacks(echo_client_cbs);
        echoclient_binary_.Connect();
        echoclient_binary_.Send("hello, from ClientThreadTest");
    }
    void OnSignal(SignalHandler* sh, uint32_t signo)
    {
        printf("ClientThreadTest(thread id:%ld) EventLoop status: %d, Shutdown\n",
                pthread_self(), EV_Singleton->IsRunning());
        EV_Singleton->StopLoop();
    }

    private:
    void OnClientConnectionReady(TcpConnection* conn)
    {
        printf("[ClientThreadTest::OnClientConnectionReady] fd: %d\n", conn->FD());
        conn->Send("hello china 2, from ClientThreadTest");
    }
    void OnMessageRecvd_Client(TcpConnection* conn, const Message* msg)
    {
        printf("[ClientThreadTest] fd: %d, message: %s, length: %lu\n", conn->FD(), msg->Payload(), msg->PayloadSize());
    }

    private:
    TcpClient echoclient_binary_;
};

class ServerThreadTest {
    public:
    ServerThreadTest() :
      echoserver_binary_("0.0.0.0", 10000, MessageType::BINARY)
    {
        TcpCallbacksPtr echo_svr_cbs = std::shared_ptr<TcpCallbacks>(new TcpCallbacks);
        echo_svr_cbs->on_msg_recvd_cb = std::bind(&ServerThreadTest::OnMessageRecvd_Server, this, std::placeholders::_1, std::placeholders::_2);
        echo_svr_cbs->on_conn_ready_cb = std::bind(&ServerThreadTest::OnServerConnectionReady, this, std::placeholders    ::_1);
        echoserver_binary_.SetTcpCallbacks(echo_svr_cbs);
    }
    void OnSignal(SignalHandler* sh, uint32_t signo)
    {
        printf("ServerThreadTest(thread id:%ld) EventLoop status: %d, Shutdown\n",
                pthread_self(), EV_Singleton->IsRunning());
        EV_Singleton->StopLoop();
    }

    private:
    void OnServerConnectionReady(TcpConnection* conn)
    {
        printf("[ServerThreadTest::OnServerConnectionReady] fd: %d\n", conn->FD());
        conn->Send("hello china 2");
    }
    void OnMessageRecvd_Server(TcpConnection* conn, const Message* msg)
    {
        printf("[ServerThreadTest::OnMessageRecvd_Server] fd: %d, message: %s, length: %lu\n", conn->FD(), msg->Payload(), msg->PayloadSize());
        conn->Send(*msg);
    }

    private:
    TcpServer echoserver_binary_;
};

}  // ns evt_loop

using namespace evt_loop;

void* ClientThreadStack(void* arg) {
  printf(">>> ClientThreadStack start, thread id: %ld\n", pthread_self());
  ClientThreadTest client;
  SignalHandler sh(SignalEvent::INT, std::bind(&ClientThreadTest::OnSignal, &client, std::placeholders::_1, std::placeholders::_2));

  EV_Singleton->StartLoop();

  printf(">>> ClientThreadStack exit.\n");
  return NULL;
}

int main(int argc, char **argv) {
  printf(">>> ServerThreadStack start, thread id: %ld\n", pthread_self());

  pthread_t tid;
  pthread_create(&tid, NULL, ClientThreadStack, NULL);

  ServerThreadTest server;
  //SignalHandler sh(SignalEvent::INT, std::bind(&ServerThreadTest::OnSignal, &server, std::placeholders::_1, std::placeholders::_2));
  SignalHandler sh(SignalEvent::INT, [&](SignalHandler* sh, uint32_t signo) {
          server.OnSignal(sh, signo);
          //pthread_kill(tid, signo);
          });

  //std::thread client_thread(ClientThreadStack);

  EV_Singleton->StartLoop();

  //client_thread.detach();
  //pthread_join(tid, NULL);
  pthread_detach(tid);

  printf(">>> ServerThreadStack exit.\n");
  return 0;
}

