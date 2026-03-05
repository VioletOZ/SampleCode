#pragma once
#include "stdafx.h"
#include "Room.h"


class RoomManager
{
public:
    RoomManager();

    RoomPtr GetRoom(uint32_t room_id) {
        auto it = _rooms.find(room_id);
        return (it != _rooms.end()) ? it->second : RoomPtr();
    }

    void HandleCreateRoom(SessionPtr session, const uint32_t user_id);

    void HandleMessage(SessionPtr session, const Message& message);	
    void HandleJoinRoom(SessionPtr session, const Message& message);
    void HandleLeaveRoom(SessionPtr session, const Message& message);
    void HandleChatMessage(SessionPtr session, const Message& message);
    void HandleRoomListRequest(SessionPtr session, const Message& message);
    void HandleUserListRequest(SessionPtr session, const Message& message);

    std::map<uint32_t, RoomPtr> GetAllRooms() const { return _rooms; }

private:
    std::map<uint32_t, RoomPtr> _rooms;    
};