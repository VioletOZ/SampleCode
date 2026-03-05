#include "stdafx.h"
#include "RoomManager.h"
#include "ServerCommon.h"

RoomManager::RoomManager()
{
	std::cout << "RoomManager initialized." << std::endl;
}

void RoomManager::HandleMessage(SessionPtr session, const Message& message) 
{
    switch (message.type) {
    case MSG_CREATE_ROOM:
		HandleCreateRoom(session, message.user_id);
        break;
    case MSG_JOIN_ROOM:
        HandleJoinRoom(session, message);
        break;
    case MSG_LEAVE_ROOM:
        HandleLeaveRoom(session, message);
        break;
    case MSG_CHAT:
        HandleChatMessage(session, message);
        break;
    case MSG_ROOM_LIST:
        HandleRoomListRequest(session, message);
        break;
    case MSG_USER_LIST:
        HandleUserListRequest(session, message);
        break;
    default:
        std::cout << "Unknown message type: " << message.type << std::endl;
    }
}

void RoomManager::HandleCreateRoom(SessionPtr session, const uint32_t user_id)
{
    // 이미 방에 있으면 생성불가
    if (session->GetCurrentRoomId() > 0) {        
        /*google::protobuf::Message response;
        response.type = MSG_CREATE_ROOM;
        response.room_id = session->GetCurrentRoomId();
        response.user_id = session->GetUserId();
        std::string success_msg = "Already in the room " + std::to_string(session->GetCurrentRoomId());
        response.data_length = success_msg.length();
        strcpy(response.data, success_msg.c_str());
        session->SendMsg(response);*/
        return;
    }

    uint32_t room_id = GetRandomNumber(1, 9999);

    while (GetRoom(room_id))
    {
        room_id = GetRandomNumber(1, 9999);
    }

    std::string room_name = "Room_" + std::to_string(room_id);
    auto room = boost::make_shared<Room>(room_id, room_name);

    _rooms[room_id] = room;

    Message message;
    message.user_id = user_id;
    message.room_id = room_id;

    std::cout << "Room created: " << room_id << " - " << room_name << std::endl;
    room->Join(session);

    // 입장 확인 메시지 전송
    Message response;
    response.type = MSG_CREATE_ROOM;
    response.room_id = message.room_id;
    response.user_id = session->GetUserId();
    std::string success_msg = "Successfully created room " + room->GetName();
    response.data_length = success_msg.length();
    strcpy(response.data, success_msg.c_str());
    session->SendMsg(response);
    //return room;
}

void RoomManager::HandleJoinRoom(SessionPtr session, const Message& message)
{
    // 현재 룸에 있으니 입장못함
    if (session->GetCurrentRoomId() > 0) {
        /*Message response;
        response.type = MSG_JOIN_ROOM;
        response.room_id = session->GetCurrentRoomId();
        response.user_id = session->GetUserId();
        std::string success_msg = "Need first leave room " + std::to_string(message.room_id);
        response.data_length = success_msg.length();
        strcpy(response.data, success_msg.c_str());
        session->SendMsg(response);*/
        return;
    }

    auto room = GetRoom(message.room_id);
    // 입장하려는 룸이 없음
    if (!room) {
        /*Message response;
        response.type = MSG_JOIN_ROOM;
        response.room_id = session->GetCurrentRoomId();
        response.user_id = session->GetUserId();
        std::string success_msg = "Not exists room id " + std::to_string(message.room_id) + " execute -> /rooms";
        response.data_length = success_msg.length();
        strcpy(response.data, success_msg.c_str());
        session->SendMsg(response);*/
        return;
    }

    /*if (session->GetCurrentRoomId() > 0) {
        auto current_room = GetRoom(session->GetCurrentRoomId());
        if (current_room) {
            current_room->Leave(session);
        }
    }*/

    // 새 룸에 입장
    room->Join(session);

    // 입장 확인 메시지 전송
    /*Message response;
    response.type = MSG_JOIN_ROOM;
    response.room_id = message.room_id;
    response.user_id = session->GetUserId();
    std::string success_msg = "Successfully joined room " + room->GetName();
    response.data_length = success_msg.length();
    strcpy(response.data, success_msg.c_str());
    session->SendMsg(response);*/
}

void RoomManager::HandleLeaveRoom(SessionPtr session, const Message& message)
{
    auto room = GetRoom(session->GetCurrentRoomId());
    if (room) {
        room->Leave(session);

        /*Message response;
        response.type = MSG_LEAVE_ROOM;
        response.room_id = message.room_id;
        response.user_id = session->GetUserId();
        std::string leave_msg = "Left room " + room->GetName();
        response.data_length = leave_msg.length();
        strcpy(response.data, leave_msg.c_str());
        session->SendMsg(response);*/
    }
}

void RoomManager::HandleChatMessage(SessionPtr session, const Message& message)
{
    auto room = GetRoom(session->GetCurrentRoomId());
    if (room) {
        Message broadcast_msg = message;
        broadcast_msg.user_id = session->GetUserId();
        //room->Broadcast(broadcast_msg);
        std::cout << "Chat in room " << room->GetId() << " from user "
            << session->GetUserId() << ": " << message.data << std::endl;
    }
}

void RoomManager::HandleRoomListRequest(SessionPtr session, const Message& message)
{
    Message response;
    response.type = MSG_ROOM_LIST;
    response.user_id = session->GetUserId();

    std::string room_list = "Available rooms:\n";
    auto rooms = GetAllRooms();
    for (const auto& pair : rooms) {
        auto room = pair.second;
        room_list += std::to_string(room->GetId()) + ": " + room->GetName()
            + " (" + std::to_string(room->GetMemberCount()) + " users)\n";
    }

    response.data_length = room_list.length();
    strcpy(response.data, room_list.c_str());
    //session->SendMsg(response);
}

void RoomManager::HandleUserListRequest(SessionPtr session, const Message& message)
{
    auto room = GetRoom(session->GetCurrentRoomId());
    if (room) {
        /*Message response;
        response.type = MSG_USER_LIST;
        response.room_id = room->GetId();
        response.user_id = session->GetUserId();

        std::string user_list = "Users in room " + room->GetName() + ":\n";
        auto member_ids = room->GetMemberIds();
        for (uint32_t id : member_ids) {
            user_list += "User " + std::to_string(id) + "\n";
        }

        response.data_length = user_list.length();
        strcpy(response.data, user_list.c_str());
        session->SendMsg(response);*/
    }
}