#pragma once
#include "stdafx.h"

#include <cstdint>

#include "proto/Common.pb.h"  // protobuf::PacketType

#pragma pack(push, 1)
struct PacketHeader {
    uint32_t body_size;         // 패킷 크기
    uint16_t type;              // 메시지 타입
};

#pragma pack(pop)

static constexpr size_t HEADER_SIZE = sizeof(PacketHeader);     // 6 bytes
static constexpr size_t MAX_PACKET_SIZE = 1024 * 1024;          // 1MB
