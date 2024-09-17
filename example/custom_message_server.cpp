#include <stdio.h>

#include "eventloop/el.h"
#include "custom_message.h"

using namespace evt_loop;

class CustomMessageServer {
    public:
    CustomMessageServer() :
      server_("0.0.0.0", 10000, MessageType::CUSTOM)
    {
        auto msg_hdr_desc = CreateMessageHeaderDescription();
        server_.SetMessageHeaderDescription(msg_hdr_desc);

        TcpCallbacksPtr svr_cbs = std::shared_ptr<TcpCallbacks>(new TcpCallbacks);
        svr_cbs->on_msg_recvd_cb = std::bind(&CustomMessageServer::OnMessageRecvd, this, std::placeholders::_1, std::placeholders::_2);
        svr_cbs->on_conn_ready_cb = std::bind(&CustomMessageServer::OnConnectionReady, this, std::placeholders::_1);
        server_.SetTcpCallbacks(svr_cbs);
    }
    void OnSignal(SignalHandler* sh, uint32_t signo)
    {
        printf("Shutdown\n");
        EV_Singleton->StopLoop();
    }

    private:
    void OnConnectionReady(TcpConnection* conn)
    {
        printf("[OnConnectionReady] fd: %d\n", conn->FD());

        CommandMessage msg;
        const char* content = "hello china from server";
        size_t msg_payload_len = server_.GetMessageHeaderDescription()->is_payload_len_including_self ? sizeof(msg.payload_len) + strlen(content) : strlen(content);
        msg.cmd = 255;
        msg.payload_len = htonl(msg_payload_len);
        conn->Send((char*)&msg, sizeof(msg));
        conn->Send(content);
    }

    void OnMessageRecvd(TcpConnection* conn, const Message* msg)
    {
        printf("[server] fd: %d, message: %s, length: %lu\n", conn->FD(), msg->Payload(), msg->PayloadSize());
        printf("[server] msg size: %lu\n", msg->Size());
        printf("received msg bytes:\n");
        msg->DumpHex();

        conn->Send(*msg);

        CommandMessage* cmdMsg = (CommandMessage*)(msg->Data().data());
        cmdMsg->payload_len = ntohl(cmdMsg->payload_len);
        printf("Received command, cmd: %d\n", cmdMsg->cmd);
        printf("Received command, payload_len: %d\n", cmdMsg->payload_len);
    }

    HeaderDescriptionPtr CreateMessageHeaderDescription() {
        auto msg_hdr_desc = std::make_shared<HeaderDescription>();
        msg_hdr_desc->hdr_len = sizeof(CommandMessage);
        msg_hdr_desc->payload_len_offset = 1;  // jump a byte of cmd field
        msg_hdr_desc->payload_len_bytes = sizeof(CommandMessage::payload_len);
        msg_hdr_desc->is_payload_len_including_self = true;
        return msg_hdr_desc;
    }

    private:
    TcpServer server_;
};

int main(int argc, char **argv) {
  CustomMessageServer server;
  SignalHandler sh(SignalEvent::INT, std::bind(&CustomMessageServer::OnSignal, &server, std::placeholders::_1, std::placeholders::_2));

  EV_Singleton->StartLoop();

  return 0;
}

