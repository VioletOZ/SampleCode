#pragma once
#include <iostream>

#include "proto/UserPacket.pb.h"
#include "proto/RoomPacket.pb.h"
#include "proto/ChatPacket.pb.h"
#include "proto/Common.pb.h"

struct RawPacket {
    uint16_t    type = 0;   // 연결 종료 이벤트(IncomingMessage)에서는 0
    std::string body;
};
