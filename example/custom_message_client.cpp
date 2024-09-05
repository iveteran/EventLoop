#include <stdio.h>
#include <arpa/inet.h>
#include "el.h"
#include "custom_message.h"

using namespace evt_loop;

class CustomMessageClient {
    public:
    CustomMessageClient() :
        client_("localhost", 10000, MessageType::CUSTOM)
    {
        auto msg_hdr_desc = CreateMessageHeaderDescription();
        client_.SetMessageHeaderDescription(msg_hdr_desc);

        TcpCallbacksPtr client_cbs = std::shared_ptr<TcpCallbacks>(new TcpCallbacks);
        client_cbs->on_msg_recvd_cb = std::bind(&CustomMessageClient::OnMessageRecvd, this, std::placeholders::_1, std::placeholders::_2);

        client_.SetNewClientCallback(std::bind(&CustomMessageClient::OnConnectionCreated, this, std::placeholders::_1));
        client_.SetTcpCallbacks(client_cbs);

        client_.Connect();

        CommandMessage msg;
        const char* content = "hello world from client";
        size_t msg_payload_len = msg_hdr_desc->is_payload_len_including_self ? sizeof(msg.payload_len) + strlen(content) : strlen(content);
        msg.cmd = 1;
        msg.payload_len = htonl(msg_payload_len);

        string msg_bytes((char*)&msg, sizeof(msg));
        msg_bytes.append(content);
        size_t msg_length = sizeof(msg) + strlen(content);
        printf("hdr size: %ld\n", sizeof(msg));
        printf("msg size: %ld\n", msg_length);
        printf("msg bytes:\n");
        DumpHex(msg_bytes);

        client_.Send((char*)&msg, sizeof(msg));
        client_.Send(content);
    }

    protected:
    void OnMessageRecvd(TcpConnection* conn, const Message* msg)
    {
        printf("[OnMessageRecvd] received message, fd: %d, message: %s, length: %lu\n", conn->FD(), msg->Payload(), msg->PayloadSize());
        printf("[client] msg size: %lu\n", msg->Size());
        printf("[client] msg bytes:\n");
        msg->DumpHex();

        CommandMessage* cmdMsg = (CommandMessage*)(msg->Data().data());
        cmdMsg->payload_len = ntohl(cmdMsg->payload_len);
        printf("Received command, cmd: %d\n", cmdMsg->cmd);
        printf("Received command, payload_len: %d\n", cmdMsg->payload_len);
    }
    void OnConnectionCreated(TcpConnection* conn)
    {
        printf("[OnConnectionCreated] connection created, fd: %d\n", conn->FD());
        printf("[OnConnectionCreated] ping\n");

        CommandMessage msg;
        const char* content = "ping";
        size_t msg_payload_len = client_.GetMessageHeaderDescription()->is_payload_len_including_self ? sizeof(msg.payload_len) + strlen(content) : strlen(content);
        msg.cmd = 2;
        msg.payload_len = htonl(msg_payload_len);

        printf("hdr bytes:\n");
        string hdr_bytes((char*)&msg, sizeof(msg));
        DumpHex(hdr_bytes);

        client_.Send((char*)&msg, sizeof(msg));
        client_.Send(content);
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
    TcpClient client_;
};

int main(int argc, char **argv) {
  CustomMessageClient client;

  SignalHandler sh(SignalEvent::INT, [&](SignalHandler* sh, uint32_t signo) {
          printf("Shutdown\n");
          EV_Singleton->StopLoop();
          });

  EV_Singleton->StartLoop();

  return 0;
}

