#include "stdafx.h"
#include "Room/RoomManager.h"
#include "Common/ServerCommon.h"
#include "Network/ResponseUtils.h"
#include <ranges>

RoomManager::RoomManager(LobbyManager& lobby)
    : _lobby(lobby)
{
}

void RoomManager::EnterRoom(const SessionPtr& session, Room& room)
{
    _lobby.EnterRoom(session);
    room.Join(session);
}

void RoomManager::HandleCreateRoom(SessionPtr session, const protobuf::C_RoomCreate& request)
{
    if (session->GetCurrentRoomId() > 0) {
        LogDebug("Already in room ", session->GetCurrentRoomId());
        SendStatus<protobuf::S_RoomCreate>(session, protobuf::ROOM_CREATE, protobuf::ERR_ALREADY_IN_ROOM);
        return;
    }

    const uint32_t room_id = AllocateRoomId();

    std::string room_name = session->GetUserName() + "_" + std::to_string(room_id);
    auto room = std::make_shared<Room>(room_id, room_name);
    _rooms[room_id] = room;

    protobuf::S_RoomCreate response;
    response.mutable_room_info()->set_room_id(room_id);
    response.mutable_room_info()->set_room_type(request.room_type());
    response.mutable_room_info()->set_room_name(room_name);
    response.set_status_code(protobuf::OK);

    session->SendPacket(PacketUtils::Serialize(protobuf::ROOM_CREATE, response));

    LogDebug("Room created: ", room_id, " - ", room_name);
    EnterRoom(session, *room);
}

void RoomManager::HandleJoinRoom(SessionPtr session, const protobuf::C_RoomJoin& request)
{
    if (session->GetCurrentRoomId() > 0) {
        LogDebug("Need to leave current room first");
        SendStatus<protobuf::S_RoomJoin>(session, protobuf::ROOM_JOIN, protobuf::ERR_ALREADY_IN_ROOM);
        return;
    }

    Room* room = FindRoom(request.room_id());
    const protobuf::StatusCode status = !room ? protobuf::ERR_ROOM_NOT_FOUND
                                      : room->IsFull() ? protobuf::ERR_ROOM_FULL
                                      : !room->IsJoinable() ? protobuf::ERR_GAME_IN_PROGRESS   // 카운트다운/진행 중
                                      : protobuf::OK;
    if (status != protobuf::OK) {
        LogDebug("Room join rejected: ", request.room_id(), " ", protobuf::StatusCode_Name(status));
        SendStatus<protobuf::S_RoomJoin>(session, protobuf::ROOM_JOIN, status);
        return;
    }

    EnterRoom(session, *room);

    protobuf::S_RoomJoin response;
    response.set_status_code(protobuf::OK);
    response.mutable_room_info()->set_room_id(room->GetRoomId());
    response.mutable_room_info()->set_room_name(room->GetRoomName());

    session->SendPacket(PacketUtils::Serialize(protobuf::ROOM_JOIN, response));
}

bool RoomManager::LeaveCurrentRoom(SessionPtr session)
{
    Room* room = FindRoom(session->GetCurrentRoomId());
    if (!room || !room->Leave(session)) {
        return false;
    }
    _lobby.ReturnFromRoom(session);   // 로그아웃/종료로 이미 채널을 반환한 세션이면 무시됨

    // 마지막 사람이 나가면 방 삭제(삭제 후 room 사용 금지)
    if (room->GetMemberCount() == 0) {
        LogDebug("Room removed (empty): ", room->GetRoomId(), " - ", room->GetRoomName());
        _rooms.erase(room->GetRoomId());
    }
    return true;
}

void RoomManager::HandleLeaveRoom(SessionPtr session, const protobuf::C_RoomLeave& request)
{
    SendStatus<protobuf::S_RoomLeave>(session, protobuf::ROOM_LEAVE,
                                      LeaveCurrentRoom(session) ? protobuf::OK : protobuf::ERR_NOT_IN_ROOM);
}

// 순차 발급: 32비트를 한 바퀴 돌면 0과 사용 중인 번호를 건너뜀(방이 2^32-1개 미만이면 반드시 종료).
// 이전의 무작위 1~9999 + 재시도는 방이 9999개가 되면 무한 루프였음
uint32_t RoomManager::AllocateRoomId()
{
    do {
        ++_next_room_id;
    } while (_next_room_id == 0 || _rooms.count(_next_room_id));
    return _next_room_id;
}

void RoomManager::HandleChatMessage(SessionPtr session, const protobuf::C_ChatPacket& request)
{
    std::optional<std::string> packet = SerializeChatBroadcast(session, request, protobuf::CHAT_ROOM);
    if (!packet) return;

    if (Room* room = FindCurrentRoom<protobuf::S_ChatPacket>(session, protobuf::CHAT_MESSAGE)) room->Broadcast(std::move(*packet));
}

void RoomManager::HandleGameReady(SessionPtr session, const protobuf::C_GameReady& request)
{
    Room* room = FindCurrentRoom<protobuf::S_GameReady>(session, protobuf::GAME_READY);
    if (!room) return;

    protobuf::S_GameReady response;
    response.set_status_code(room->GetGame().SetReady(room->GetPlayerId(session), request.ready()));
    response.set_ready(request.ready());
    session->SendPacket(PacketUtils::Serialize(protobuf::GAME_READY, response));
}

// 받아들인 이동은 응답 없음(다음 스냅샷이 확인 역할). 거부 시에만 서버 위치를 담아 응답
void RoomManager::HandleGameMove(SessionPtr session, const protobuf::C_GameMove& request)
{
    Room* room = FindCurrentRoom<protobuf::S_GameMove>(session, protobuf::GAME_MOVE);
    if (!room) return;

    const RoomGame::MoveResult result = room->GetGame().TryMove(room->GetPlayerId(session), request.x(), request.y(), request.seq(),
                                                                std::chrono::steady_clock::now());
    if (result.status == protobuf::OK) return;

    LogDebug("Game move rejected: ", session->GetUserName(), " room ", room->GetRoomId(), " ", protobuf::StatusCode_Name(result.status));
    protobuf::S_GameMove response;
    response.set_status_code(result.status);
    response.set_x(result.x);
    response.set_y(result.y);
    response.set_seq(request.seq());
    session->SendPacket(PacketUtils::Serialize(protobuf::GAME_MOVE, response));
}

template<typename Response>
Room* RoomManager::FindCurrentRoom(const SessionPtr& session, protobuf::PacketType type)
{
    Room* room = FindRoom(session->GetCurrentRoomId());
    if (!room) {   // 방 안 상태인데 방이 없음 — 조용히 버리지 않고 알림
        LogDebug("Room ", session->GetCurrentRoomId(), " not found for ", session->GetUserName(), " (type ", static_cast<int>(type), ")");
        SendStatus<Response>(session, type, protobuf::ERR_NOT_IN_ROOM);
    }
    return room;
}

void RoomManager::HandleRoomList(SessionPtr session, const protobuf::C_RoomList& request)
{
    const uint32_t total_rooms = static_cast<uint32_t>(_rooms.size());
    const uint32_t total_pages = (total_rooms + ROOM_LIST_PAGE_SIZE - 1) / ROOM_LIST_PAGE_SIZE;
    const uint32_t page = std::clamp(request.page(), 1u, std::max(total_pages, 1u));   // 0 -> 1, 범위 초과 -> 마지막

    protobuf::S_RoomList response;
    response.set_page(page);
    response.set_total_pages(total_pages);
    response.set_total_rooms(total_rooms);
    // _rooms는 방 번호 순 std::map -> 앞 페이지만큼 건너뛴 뒤 최대 ROOM_LIST_PAGE_SIZE개
    auto it = std::next(_rooms.begin(), std::min<size_t>(_rooms.size(), static_cast<size_t>(page - 1) * ROOM_LIST_PAGE_SIZE));
    for (uint32_t i = 0; i < ROOM_LIST_PAGE_SIZE && it != _rooms.end(); ++i, ++it) {
        auto* info = response.add_room_list();
        info->set_room_id(it->second->GetRoomId());
        info->set_room_name(it->second->GetRoomName());
    }

    response.set_status_code(protobuf::OK);
    session->SendPacket(PacketUtils::Serialize(protobuf::ROOM_LIST, response));
}

void RoomManager::HandleUserList(SessionPtr session, const protobuf::C_RoomInfo& request)
{
    // room_id 0 = 내 방(방 안에서 요청한 경우. 로비에서 0이면 GameLoop가 LobbyManager로 보냄)
    const uint32_t room_id = request.room_info().room_id() != 0 ? request.room_info().room_id() : session->GetCurrentRoomId();
    const Room* room = FindRoom(room_id);
    if (!room) {
        SendStatus<protobuf::S_RoomInfo>(session, protobuf::ROOM_USER_LIST, protobuf::ERR_ROOM_NOT_FOUND);
        return;
    }

    protobuf::S_RoomInfo response;
    response.mutable_room_info()->set_room_id(room->GetRoomId());
    response.mutable_room_info()->set_room_name(room->GetRoomName());
    AppendUserNames(std::views::keys(room->GetMembers()), response);
    response.set_status_code(protobuf::OK);
    session->SendPacket(PacketUtils::Serialize(protobuf::ROOM_USER_LIST, response));
}

void RoomManager::Update(std::chrono::steady_clock::time_point now)
{
    for (auto& [id, room] : _rooms) {
        if (std::string state = room->UpdateGame(now); !state.empty()) room->Broadcast(std::move(state));
        if (std::string snapshot = room->TakeSnapshot(); !snapshot.empty()) room->Broadcast(std::move(snapshot));
    }
}
