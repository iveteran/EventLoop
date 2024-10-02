#include <stdio.h>

#include "custom_message.h"
#include "eventloop/el.h"
#include "eventloop/logger.h"

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
        el_logger->info("[CustomMessageServer::OnSignal] Shutdown");
        EV_Singleton->StopLoop();
    }

    private:
    void OnConnectionReady(TcpConnection* conn)
    {
        el_logger->debug("[CustomMessageServer::OnConnectionReady] fd: {}", conn->FD());

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
        el_logger->debug("[CustomMessageServer::OnMessageRecvd] fd: {}, message: {}, length: {}",
                conn->FD(), msg->Payload(), msg->PayloadSize());
        el_logger->debug("[CustomMessageServer::OnMessageRecvd] msg size: {}", msg->Size());
        el_logger->debug("[CustomMessageServer::OnMessageRecvd] received msg bytes:");
        el_logger->output(msg->DumpHex()).eol();

        conn->Send(*msg);

        CommandMessage* cmdMsg = (CommandMessage*)(msg->Data().data());
        cmdMsg->payload_len = ntohl(cmdMsg->payload_len);
        // NOTE: will print blank for uint8_t variable, MUST add "+" before it;
        //   refer: https://stackoverflow.com/questions/19562103/uint8-t-cant-be-printed-with-cout
        el_logger->debug("[CustomMessageServer::OnMessageRecvd] Received command, cmd: {}", +cmdMsg->cmd);
        el_logger->debug("[CustomMessageServer::OnMessageRecvd] Received command, payload_len: {}", cmdMsg->payload_len);
    }

    HeaderDescriptionPtr CreateMessageHeaderDescription() {
        auto msg_hdr_desc = std::make_shared<HeaderDescription>();
        msg_hdr_desc->hdr_len = sizeof(CommandMessage);
        msg_hdr_desc->payload_len_offset = 1;  // jump a byte of cmd field
        msg_hdr_desc->payload_len_bytes = sizeof(CommandMessage::payload_len);
        msg_hdr_desc->is_payload_len_including_self = true;
        auto hb_request = create_heartbeat_request(msg_hdr_desc->is_payload_len_including_self);
        msg_hdr_desc->heartbeat_request = string((char*)&hb_request, sizeof(hb_request));
        auto hb_response = create_heartbeat_response(msg_hdr_desc->is_payload_len_including_self);
        msg_hdr_desc->heartbeat_response = string((char*)&hb_response, sizeof(hb_response));
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

