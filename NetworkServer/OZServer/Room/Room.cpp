#include "stdafx.h"
#include "Room/Room.h"

void Room::Join(SessionPtr session)
{
    const uint32_t player_id = ++_next_player_id;
    _members.emplace(session, player_id);
    _game.AddPlayer(player_id, session->GetUserName());
    session->SetCurrentRoomId(_id);
    session->SetState(SessionState::InRoom);

    protobuf::C_ChatPacket notice;
    notice.set_chat_type(protobuf::CHAT_ROOM);
    notice.mutable_user_info()->set_name(session->GetUserName());
    notice.set_message("User " + session->GetUserName() + " joined the room");

    Broadcast(PacketUtils::Serialize(protobuf::CHAT_MESSAGE, notice));
    LogDebug("User ", session->GetUserName(), " joined room ", _id);
}

bool Room::Leave(SessionPtr session_ptr)
{
    auto it = _members.find(session_ptr);
    if (it == _members.end()) {
        LogDebug("User ", session_ptr->GetSessionId(), " is not in room ", _id);
        return false;
	}
    auto session = it->first;

    _game.RemovePlayer(it->second);   // 진행 중이면 남은 인원에게 다음 스냅샷으로 알림, 인원 부족 시 다음 틱에 결과
    _members.erase(it);
    session->SetCurrentRoomId(0);
    session->SetState(SessionState::Lobby);

    protobuf::C_ChatPacket notice;
    notice.set_chat_type(protobuf::CHAT_ROOM);
    notice.mutable_user_info()->set_name(session->GetUserName());
    notice.set_message("User " + session->GetUserName() + " left the room");

    Broadcast(PacketUtils::Serialize(protobuf::CHAT_MESSAGE, notice));
    LogDebug("User ", session->GetUserName(), " left room ", _id);

    return true;
}

void Room::Broadcast(std::string message)
{
    auto shared_msg = std::make_shared<const std::string>(std::move(message));
    for (const auto& [session, player_id] : _members) {   // 참조로 순회: 멤버마다 shared_ptr 참조 카운트 증감(atomic) 없음
        session->SendPacket(shared_msg);
    }
}

std::string Room::UpdateGame(std::chrono::steady_clock::time_point now)
{
    if (!_game.Update(now)) return {};

    // 진행 중에만 InGame(이동 허용) — 패킷 허용 여부는 GameLoop 등록의 허용 상태로 판정
    const SessionState state = (_game.GetPhase() == protobuf::GAME_PLAYING) ? SessionState::InGame : SessionState::InRoom;
    for (const auto& [session, player_id] : _members) session->SetState(state);

    protobuf::S_GameState message;
    _game.BuildState(now, message);
    return PacketUtils::Serialize(protobuf::GAME_STATE, message);
}

std::string Room::TakeSnapshot()
{
    if (_game.GetPhase() != protobuf::GAME_PLAYING) {
        _tick = 0;
        return {};
    }
    protobuf::S_GameSnapshot snapshot;
    if (++_tick % GAME_SNAPSHOT_EVERY_TICKS != 0 || !_game.BuildSnapshot(snapshot)) return {};
    return PacketUtils::Serialize(protobuf::GAME_SNAPSHOT, snapshot);
}
