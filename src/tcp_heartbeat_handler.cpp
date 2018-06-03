#include "tcp_heartbeat_handler.h"
#include "tcp_connection.h"

namespace evt_loop {

#define BINARY_HEARTBEAT_REQUEST_MAGIC   0x4842      // "HB"
#define BINARY_HEARTBEAT_RESPONSE_MAGIC  0x4248      // "BH"

#pragma pack(1)
struct DefaultBinaryHeartbeatMessage
{
    uint32_t length;
    uint16_t magic;
    DefaultBinaryHeartbeatMessage(uint16_t _magic) : length(sizeof(*this)), magic(_magic) { }
};
#pragma pack()

static const DefaultBinaryHeartbeatMessage dft_binary_heartbeat_request(BINARY_HEARTBEAT_REQUEST_MAGIC);
static const DefaultBinaryHeartbeatMessage dft_binary_heartbeat_response(BINARY_HEARTBEAT_RESPONSE_MAGIC);
static const string dft_json_heartbeat_request = "{ \"heartbeat\": \"request\" }";
static const string dft_json_heartbeat_response = "{ \"heartbeat\": \"response\" }";
static const string dft_crlf_heartbeat_request = "heartbeat_reqeust\r\n";
static const string dft_crlf_heartbeat_response = "heartbeat_response\r\n";

void PrintMessage(const Message* msg)
{
    if (msg->Type() == MessageType::BINARY) {
        msg->DumpHex();
    } else {
        printf("%s\n", msg->Data().c_str());
    }

}

TcpHeartbeatHandler::HeartbeatPing::HeartbeatPing(TcpHeartbeatHandler* hb_hdlr,
        uint32_t idle_interval, uint32_t ping_interval, uint32_t ping_total) :
    hb_hdlr_(hb_hdlr),
    m_idle_interval(idle_interval), m_ping_interval(ping_interval), m_ping_total(ping_total), m_ping_count(0),
    m_idle_timer(std::bind(&HeartbeatPing::OnIdleTimer, this, std::placeholders::_1)),
    m_ping_timer(std::bind(&HeartbeatPing::OnPingTimer, this, std::placeholders::_1))
{
    TimeVal tv(m_idle_interval, 0);
    m_idle_timer.SetInterval(tv);
    m_idle_timer.Start();

    TimeVal tv2(m_ping_interval, 0);
    m_ping_timer.SetInterval(tv2);
}

TcpHeartbeatHandler::HeartbeatPing::~HeartbeatPing()
{
    m_idle_timer.Stop();
    m_ping_timer.Stop();
}

void TcpHeartbeatHandler::HeartbeatPing::OnIdleTimer(TimerEvent* timer)
{
    if (! m_ping_timer.IsRunning())
        m_ping_timer.Start();
}

void TcpHeartbeatHandler::HeartbeatPing::OnPingTimer(TimerEvent* timer)
{
    if (m_ping_count < m_ping_total)
    {
        hb_hdlr_->m_send_heartbeat_request_cb(hb_hdlr_->m_connection);
        m_ping_count++;
    }
    else
    {
        hb_hdlr_->m_heartbeat_timeout_cb(hb_hdlr_->m_connection);
        OnFinishPing();
    }
}

void TcpHeartbeatHandler::HeartbeatPing::OnFinishPing()
{
    if (m_ping_timer.IsRunning())
    {
        m_ping_timer.Stop();
    }
    m_ping_count = 0;
}

TcpHeartbeatHandler::TcpHeartbeatHandler(TcpConnection* conn) : m_connection(conn)
{
    SetCallbacks();
}

void TcpHeartbeatHandler::SetCallbacks(const SendHeartbeatRequestCallback& send_hb_req_cb,
        const SendHeartbeatRequestCallback& send_hb_rsp_cb,
        const HeartbeatTimeoutCallback& hb_timeout_cb)
{
    m_send_heartbeat_request_cb = send_hb_req_cb ? send_hb_req_cb : std::bind(&TcpHeartbeatHandler::SendHeartbeatRequest, this, std::placeholders::_1);
    m_send_heartbeat_response_cb = send_hb_rsp_cb ? send_hb_rsp_cb : std::bind(&TcpHeartbeatHandler::SendHeartbeatResponse, this, std::placeholders::_1);
    m_heartbeat_timeout_cb = hb_timeout_cb ? hb_timeout_cb : std::bind(&TcpHeartbeatHandler::OnConnectionDead, this, std::placeholders::_1);
}

void TcpHeartbeatHandler::EnablePing(uint32_t idle_interval, uint32_t ping_interval, uint32_t ping_total)
{
    m_heartbeat_pinger = std::make_shared<HeartbeatPing>(this, idle_interval, ping_interval, ping_total);
}

void TcpHeartbeatHandler::DisablePing()
{
    m_heartbeat_pinger = nullptr;
}

bool TcpHeartbeatHandler::IsHeartbeatRequest(const Message* msg)
{
    bool retval = false;
    switch (msg->Type())
    {
        case MessageType::BINARY:
            retval = IsBinaryHeartbeatRequest(msg); break;
        case MessageType::JSON:
            retval = IsJsonHeartbeatRequest(msg); break;
        case MessageType::CRLF:
            retval = IsCRLFHeartbeatRequest(msg); break;
        default:
            break;
    }
    return retval;
}

bool TcpHeartbeatHandler::IsHeartbeatResponse(const Message* msg)
{
    bool retval = false;
    switch (msg->Type())
    {
        case MessageType::BINARY:
            retval = IsBinaryHeartbeatResponse(msg); break;
        case MessageType::JSON:
            retval = IsJsonHeartbeatResponse(msg); break;
        case MessageType::CRLF:
            retval = IsCRLFHeartbeatResponse(msg); break;
        default:
            break;
    }
    return retval;
}

bool TcpHeartbeatHandler::IsBinaryHeartbeatRequest(const Message* msg)
{
    const DefaultBinaryHeartbeatMessage* hb_msg = (const DefaultBinaryHeartbeatMessage*)(msg->Data().data());
    return hb_msg->magic == BINARY_HEARTBEAT_REQUEST_MAGIC;
}

bool TcpHeartbeatHandler::IsJsonHeartbeatRequest(const Message* msg)
{
    return msg->Data() == dft_json_heartbeat_request;
}

bool TcpHeartbeatHandler::IsCRLFHeartbeatRequest(const Message* msg)
{
    return msg->Data() == dft_crlf_heartbeat_request;
}

bool TcpHeartbeatHandler::IsBinaryHeartbeatResponse(const Message* msg)
{
    DefaultBinaryHeartbeatMessage* hb_msg = (DefaultBinaryHeartbeatMessage*)(msg->Data().data());
    return hb_msg->magic == BINARY_HEARTBEAT_RESPONSE_MAGIC;
}

bool TcpHeartbeatHandler::IsJsonHeartbeatResponse(const Message* msg)
{
    return msg->Data() == dft_json_heartbeat_response;
}

bool TcpHeartbeatHandler::IsCRLFHeartbeatResponse(const Message* msg)
{
    return msg->Data() == dft_crlf_heartbeat_response;
}

void TcpHeartbeatHandler::OnHeartbeatRequestReceived(const Message* msg)
{
    printf("[TcpHeartbeatHandler::OnHeartbeatRequestReceived] client type: %d, fd: %d\n", m_connection->GetMessageType(), m_connection->FD());
    PrintMessage(msg);
    m_send_heartbeat_response_cb(m_connection);
}

void TcpHeartbeatHandler::OnHeartbeatResponseReceived(const Message* msg)
{
    printf("[TcpHeartbeatHandler::OnHeartbeatResponseReceived] client type: %d, fd: %d\n", m_connection->GetMessageType(), m_connection->FD());
    PrintMessage(msg);
    if (m_heartbeat_pinger) {
        m_heartbeat_pinger->OnFinishPing();
    }
}

void TcpHeartbeatHandler::SendHeartbeatRequest(TcpConnection* conn)
{
    printf("[TcpHeartbeatHandler::SendHeartbeatRequest] client type: %d, fd: %d\n", conn->GetMessageType(), conn->FD());
    switch (conn->GetMessageType())
    {
        case MessageType::BINARY:
            SendBinaryHeartbeatRequest(conn); break;
        case MessageType::JSON:
            SendJsonHeartbeatRequest(conn); break;
        case MessageType::CRLF:
            SendCRLFHeartbeatRequest(conn); break;
        default:
            printf("[TcpHeartbeatHandler::SendHeartbeatRequest] Unknown connection message type: %d", conn->GetMessageType());
            break;
    }
}
void TcpHeartbeatHandler::SendBinaryHeartbeatRequest(TcpConnection* conn)
{
    conn->Send((char*)&dft_binary_heartbeat_request, sizeof(dft_binary_heartbeat_request), BinaryMessage::HAS_HDR);
}
void TcpHeartbeatHandler::SendJsonHeartbeatRequest(TcpConnection* conn)
{
    conn->Send(dft_json_heartbeat_request);
}
void TcpHeartbeatHandler::SendCRLFHeartbeatRequest(TcpConnection* conn)
{
    conn->Send(dft_crlf_heartbeat_request);
}

void TcpHeartbeatHandler::SendHeartbeatResponse(TcpConnection* conn)
{
    printf("[TcpHeartbeatHandler::SendHeartbeatResponse] client type: %d, fd: %d\n", conn->GetMessageType(), conn->FD());
    switch (conn->GetMessageType())
    {
        case MessageType::BINARY:
            SendBinaryHeartbeatResponse(conn); break;
        case MessageType::JSON:
            SendJsonHeartbeatResponse(conn); break;
        case MessageType::CRLF:
            SendCRLFHeartbeatResponse(conn); break;
        default:
            printf("[TcpHeartbeatHandler::SendHeartbeatResponse] Unknown connection message type: %d", conn->GetMessageType());
            break;
    }
}
void TcpHeartbeatHandler::SendBinaryHeartbeatResponse(TcpConnection* conn)
{
    conn->Send((char*)&dft_binary_heartbeat_response, sizeof(dft_binary_heartbeat_response), BinaryMessage::HAS_HDR);
}
void TcpHeartbeatHandler::SendJsonHeartbeatResponse(TcpConnection* conn)
{
    conn->Send(dft_json_heartbeat_response);
}
void TcpHeartbeatHandler::SendCRLFHeartbeatResponse(TcpConnection* conn)
{
    conn->Send(dft_crlf_heartbeat_response);
}

void TcpHeartbeatHandler::OnConnectionDead(TcpConnection* conn)
{
    conn->Disconnect();
}

}  // namespace evt_loop
