#pragma once
#include "stdafx.h"

template<typename T>
class IMessage
{
    std::vector<std::byte> SerializePacket<T>(T packet);
    T DeserializePacket<T>(const std::vector<std::byte>& data);

};

