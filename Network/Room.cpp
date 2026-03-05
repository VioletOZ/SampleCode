#include "stdafx.h"
#include "Room.h"

Room::Room(uint32_t id, const std::string& name) : _id(id), _name(name)
{
}

void Room::Join(SessionPtr session)
{
    _members.insert(session);
    session->SetCurrentRoomId(_id);

    // 입장 알림 메시지 브로드캐스트
    Message msg;
    msg.type = MSG_CHAT;
    msg.room_id = _id;
    msg.user_id = session->GetUserId();
    std::string join_msg = "User " + std::to_string(session->GetUserId()) + " joined the room";
    msg.data_length = join_msg.length();
    strcpy(msg.data, join_msg.c_str());

    //Broadcast(msg);
    std::cout << "User " << session->GetUserId() << " joined room " << _id << std::endl;
}

void Room::Leave(SessionPtr session)
{
    _members.erase(session);
    session->SetCurrentRoomId(0);

    // 퇴장 알림 메시지 브로드캐스트
    Message msg;
    msg.type = MSG_CHAT;
    msg.room_id = _id;
    msg.user_id = session->GetUserId();
    std::string leave_msg = "User " + std::to_string(session->GetUserId()) + " left the room";
    msg.data_length = leave_msg.length();
    strcpy(msg.data, leave_msg.c_str());

    //Broadcast(msg);
    std::cout << "User " << session->GetUserId() << " left room " << _id << std::endl;
}

void Room::Broadcast(const google::protobuf::Message& message)
{
    for (auto session : _members) {
        session->SendMsg(message);
    }
}

std::vector<uint32_t> Room::GetMemberIds() const
{
    std::vector<uint32_t> ids;
    for (auto session : _members) {
        ids.push_back(session->GetUserId());
    }
    return ids;
}