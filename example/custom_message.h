#ifndef _CUSTOM_MESSAGE_H_
#define _CUSTOM_MESSAGE_H_

#pragma pack(1)
struct CommandMessage {
    uint8_t cmd;
    uint32_t payload_len;   // payload_len supports including self size
    char payload[0];        // placehoder field
};
#pragma pack() 

#endif  // _CUSTOM_MESSAGE_H_
