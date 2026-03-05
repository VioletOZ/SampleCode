#pragma once
#include <iostream>

#include "proto/UserPacket.pb.h"
#include "proto/CardPacket.pb.h"

// 메시지 타입
enum MessageType {
    MSG_CREATE_ROOM = 1,
    MSG_JOIN_ROOM = 2,
    MSG_LEAVE_ROOM = 3,
    MSG_CHAT = 4,
    MSG_ROOM_LIST = 5,
    MSG_USER_LIST = 6
};

// 메시지 구조체
struct Message {
    uint32_t type;
    uint32_t room_id;
    uint32_t user_id;
    size_t data_length;
    char data[512];

    Message() : type(0), room_id(0), user_id(0), data_length(0) {
        memset(data, 0, sizeof(data));
    }
};