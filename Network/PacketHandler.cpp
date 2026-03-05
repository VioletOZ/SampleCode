#include "stdafx.h"
#include "PacketHandler.h"

std::unique_ptr<google::protobuf::Message> PacketHandler::CreateMessage(const std::string& type) const
{
    if (type == "PacketType1") {
        return std::make_unique<RoomPacket>();
    } else if (type == "PacketType2") {
        return std::make_unique<UserPacket>();
    } else if (type == "PacketType3") {
        return std::make_unique<CardPacket>();
    } else {
        std::cerr << "Unknown packet type: " << type << std::endl;
        return nullptr;
    }
}

std::string PacketHandler::SerializePacket(const std::string& type, const google::protobuf::Message& message)
{
    std::string serialized_data{ "" };

    try 
    {
        message.SerializeToString(&serialized_data);
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error serializing packet: " << type << " : " << e.what() << std::endl;        
    }

    return serialized_data;
}

std::unique_ptr<google::protobuf::Message> PacketHandler::DeserializePacket(const std::string& type, const std::string& data)
{
    // Create a new message of the appropriate type
    auto message = CreateMessage(type);
    if (!message) {
        return nullptr;
    }
    // Parse the data into the message
    if (!message->ParseFromString(data)) {
        return nullptr;
    }
    return message;
}