#ifndef _CUSTOM_MESSAGE_H_
#define _CUSTOM_MESSAGE_H_

#include <cstring>
#include <arpa/inet.h>

#pragma pack(1)
struct CommandMessage {
    uint8_t cmd;
    uint32_t payload_len;   // payload_len supports including self size
    char payload[0];        // placeholder field
};
#pragma pack() 

CommandMessage create_heartbeat_request(bool is_payload_len_including_self) {
    CommandMessage msg;
    msg.cmd = 254;
    size_t msg_payload_len = is_payload_len_including_self ? sizeof(msg.payload_len) : 0;
    msg.payload_len = htonl(msg_payload_len);
    return msg;
}

CommandMessage create_heartbeat_response(bool is_payload_len_including_self) {
    CommandMessage msg;
    msg.cmd = 255;
    size_t msg_payload_len = is_payload_len_including_self ? sizeof(msg.payload_len) : 0;
    msg.payload_len = htonl(msg_payload_len);
    return msg;
}

CommandMessage create_demo_message(bool is_payload_len_including_self, const char* content)
{
    CommandMessage msg;
    msg.cmd = 1;
    size_t msg_payload_len = is_payload_len_including_self ? sizeof(msg.payload_len) + strlen(content) : strlen(content);
    msg.payload_len = htonl(msg_payload_len);
    return msg;
}

#endif  // _CUSTOM_MESSAGE_H_
