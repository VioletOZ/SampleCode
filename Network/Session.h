#pragma once
#include "stdafx.h"

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <google/protobuf/io/zero_copy_stream_impl_lite.h>

#include "Common.h"
#include "RoomManager.h"

#include "User.h"
#include "Card.h"

class Session : public enable_shared_from_this<Session>
{
public:
    Session(boost::asio::io_context& io_context, RoomManager& room_manager)
        : _socket(io_context), _strand(boost::asio::make_strand(io_context)),
        _current_room_id(0), _room_manager(room_manager)
    {
    }
        
    boost::asio::ip::tcp::socket& socket() { return _socket; }

    void Start();
    void SendMsg(const google::protobuf::Message& message);

    const std::string GetSessionId() const { return _session_uuid; }
    void SetCurrentRoomId(uint32_t room_id) { _current_room_id = room_id; }
    const uint32_t GetCurrentRoomId() const { return _current_room_id; }


    // User
    User GetUser() const { return _user; }
    uint32_t GetUserId() const { return _user.GetId(); }

private:
    void ReadHeader();
    void HandleReadHeader(const boost::system::error_code& error);
    void HandleMessage();
    void DoSendMsg(const std::string& serialized);

    void Write();
    void HandleWrite(const boost::system::error_code& error);
    void HandleDisconnect();
    void LeaveCurrentRoom();

    uint32_t GenerateUserId() {
        static uint32_t id_counter = 1000;
        return ++id_counter;
    }

    boost::asio::ip::tcp::socket _socket;
    RoomManager& _room_manager;
    boost::asio::strand<boost::asio::io_context::executor_type> _strand;

    Message _read_msg;
    std::deque<std::string> _write_msgs;
    uint32_t _current_room_id;
    
    std::string _session_uuid;

    User _user;
};


struct MessageHeader
{
    google::protobuf::uint32 size;
    uint32_t type;
};

const int MessageHeaderSize = sizeof(MessageHeader);

inline void PrintMsg(google::protobuf::Message& msg)
{
    std::string msg_str;
    msg.DebugString();
    std::cout << msg_str << std::endl;
}

inline void PacketProcess(google::protobuf::io::CodedInputStream& input_stream)
{
    MessageHeader header;

    while (input_stream.ReadRaw(&header, MessageHeaderSize))
    {
        const void* payload_ptr = NULL;

    }
}