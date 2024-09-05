#ifndef _CUSTOM_MESSAGE_H_
#define _CUSTOM_MESSAGE_H_

#pragma pack(1)
struct CommandMessage {
    uint8_t cmd;
    uint32_t payload_len;
    char payload[0];
};
#pragma pack() 

#endif  // _CUSTOM_MESSAGE_H_
