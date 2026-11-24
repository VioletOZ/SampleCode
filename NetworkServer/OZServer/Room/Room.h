#pragma once
#include "stdafx.h"
#include "Session/Session.h"
#include "Room/RoomGame.h"

typedef std::shared_ptr<Session> SessionPtr;

class Room {
public:
    Room(uint32_t id, const std::string& name) : _id(id), _name(name), _game(id)
    {
    }

    void Join(SessionPtr session);
    bool Leave(SessionPtr session);
	void Broadcast(std::string message);
    // 매 틱(GameLoop): 게임 진행. 단계 전환·인원/준비 수 변화가 있으면 세션 상태(InRoom/InGame)를 맞추고
    // 방 전원에게 보낼 S_GameState 패킷을 반환(없으면 빈 문자열)
    std::string UpdateGame(std::chrono::steady_clock::time_point now);
    // 진행 중 GAME_SNAPSHOT_EVERY_TICKS마다 바뀐 것만 담은 S_GameSnapshot 패킷(보낼 것이 없으면 빈 문자열)
    std::string TakeSnapshot();

    RoomGame& GetGame() { return _game; }
    // 멤버의 게임 player_id(1부터). 멤버가 아니면 0 -> RoomGame이 없는 플레이어로 거부
    uint32_t GetPlayerId(const SessionPtr& session) const {
        auto it = _members.find(session);
        return (it != _members.end()) ? it->second : 0;
    }

    bool IsFull() const { return _members.size() >= ROOM_CAPACITY; }
    bool IsJoinable() const { return _game.IsJoinable(); }   // 카운트다운/진행 중이면 false

    uint32_t GetRoomId() const         { return _id; }
    const std::string& GetRoomName() const { return _name; }
    size_t GetMemberCount() const           { return _members.size(); }
    // 멤버 -> 게임 player_id(읽기 전용, 복사 없음). GameLoop 스레드에서 그 호출 안에서만 사용
    const std::map<SessionPtr, uint32_t>& GetMembers() const { return _members; }

private:
    // _members를 포함한 모든 Room 상태는 GameLoop 스레드에서만 접근(입장/퇴장/브로드캐스트/조회 모두 큐 경유) -> 락 불필요
    uint32_t _id;
    std::string _name;
    std::map<SessionPtr, uint32_t> _members;   // 멤버 -> 입장 때 발급한 게임 player_id
    RoomGame _game;                            // 게임 규칙(시드 = 방 번호)
    uint32_t _next_player_id = 0;
    uint32_t _tick = 0;                        // 진행 중 틱 수(스냅샷 주기)
};
