#include <stdio.h>
#include <arpa/inet.h>
#include "eventloop/el.h"
#include "eventloop/logger.h"
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
        el_logger->debug("[CustomMessageClient] hdr size: {}", sizeof(msg));
        el_logger->debug("[CustomMessageClient] msg size: {}", msg_length);
        el_logger->debug("[CustomMessageClient] msg bytes:");
        el_logger->output(DumpHex(msg_bytes)).eol();

        client_.Send((char*)&msg, sizeof(msg));
        client_.Send(content);
    }

    protected:
    void OnMessageRecvd(TcpConnection* conn, const Message* msg)
    {
        el_logger->debug("[CustomMessageClient::OnMessageRecvd] received message, fd: {}, message: {}, length: {}",
                conn->FD(), msg->Payload(), msg->PayloadSize());
        el_logger->debug("[CustomMessageClient::OnMessageRecvd] msg size: {}", msg->Size());
        el_logger->debug("[CustomMessageClient::OnMessageRecvd] msg bytes:");
        el_logger->output(msg->DumpHex()).eol();

        CommandMessage* cmdMsg = (CommandMessage*)(msg->Data().data());
        cmdMsg->payload_len = ntohl(cmdMsg->payload_len);
        // NOTE: will print blank for uint8_t variable, MUST add "+" before it;
        //   refer: https://stackoverflow.com/questions/19562103/uint8-t-cant-be-printed-with-cout
        el_logger->debug("[CustomMessageClient::OnMessageRecvd] Received command, cmd: {}", +cmdMsg->cmd);
        el_logger->debug("[CustomMessageClient::OnMessageRecvd] Received command, payload_len: {}", cmdMsg->payload_len);
    }
    void OnConnectionCreated(TcpConnection* conn)
    {
        el_logger->debug("[CustomMessageClient::OnConnectionCreated] connection created, fd: {}", conn->FD());
        el_logger->debug("[CustomMessageClient::OnConnectionCreated] ping");

        CommandMessage msg;
        const char* content = "ping";
        size_t msg_payload_len = client_.GetMessageHeaderDescription()->is_payload_len_including_self ? sizeof(msg.payload_len) + strlen(content) : strlen(content);
        msg.cmd = 2;
        msg.payload_len = htonl(msg_payload_len);

        string hdr_bytes((char*)&msg, sizeof(msg));
        el_logger->debug("[CustomMessageClient::OnConnectionCreated] hdr bytes:");
        el_logger->output(DumpHex(hdr_bytes)).eol();

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
          el_logger->info("[main] Shutdown");
          EV_Singleton->StopLoop();
          });

  EV_Singleton->StartLoop();

  return 0;
}

