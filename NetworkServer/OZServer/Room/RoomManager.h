#pragma once
#include "stdafx.h"
#include "Room/Room.h"
#include "Lobby/LobbyManager.h"

class RoomManager
{
public:
    explicit RoomManager(LobbyManager& lobby);

    // 방 조회(소유권 없는 포인터, 참조 카운트 증감 없음). 없으면 nullptr.
    // GameLoop 스레드에서 그 호출 안에서만 사용(방은 마지막 인원이 나가면 삭제됨)
    Room* FindRoom(uint32_t room_id) const {
        auto it = _rooms.find(room_id);
        return (it != _rooms.end()) ? it->second.get() : nullptr;
    }

    void HandleCreateRoom(SessionPtr session, const protobuf::C_RoomCreate& request);
    // 로비 -> 방 입장 공통(생성/참가): 로비 집합에서 빼고 방에 넣음
    void EnterRoom(const SessionPtr& session, Room& room);

    void HandleJoinRoom(SessionPtr session, const protobuf::C_RoomJoin& request);
    void HandleLeaveRoom(SessionPtr session, const protobuf::C_RoomLeave& request);
    // 방 채팅(방 안 상태). 로비 채팅은 LobbyManager
    void HandleChatMessage(SessionPtr session, const protobuf::C_ChatPacket& request);
    // 방 번호 순 ROOM_LIST_PAGE_SIZE(10)개 단위 페이지. 마지막 페이지보다 큰 요청은 마지막 페이지
    void HandleRoomList(SessionPtr session, const protobuf::C_RoomList& request);
    // 방 인원 목록: room_id 지정 방, 0이면 내 방. 로비 인원 목록(로비에서 0)은 LobbyManager
    void HandleUserList(SessionPtr session, const protobuf::C_RoomInfo& request);
    // 방 게임: 준비/취소(방 안), 이동(진행 중). 세션의 방을 찾아 그 방의 RoomGame이 판정
    void HandleGameReady(SessionPtr session, const protobuf::C_GameReady& request);
    void HandleGameMove(SessionPtr session, const protobuf::C_GameMove& request);
    // 현재 방에서 퇴장시키고, 마지막 인원이었다면 방을 삭제. 방에 없었으면 false(로그아웃/연결 종료 시에도 호출)
    bool LeaveCurrentRoom(SessionPtr session);
    // 매 틱: 방마다 게임 진행 후 보낼 것(S_GameState, S_GameSnapshot)이 있으면 방 전원에게(직렬화 1회 공유)
    void Update(std::chrono::steady_clock::time_point now);

private:
    uint32_t AllocateRoomId();
    // 세션이 있는 방. 없으면(정상 흐름에서는 발생 안 함) 요청 응답 타입으로 ERR_NOT_IN_ROOM을 보내고 nullptr
    template<typename Response>
    Room* FindCurrentRoom(const SessionPtr& session, protobuf::PacketType type);

    // _rooms는 GameLoop 스레드에서만 접근됨(생성/참가/퇴장/디스커넥트 전부 MessageQueue를 통해 들어옴) -> 락 불필요
    std::map<uint32_t, RoomPtr> _rooms;
    LobbyManager& _lobby;         // 방 입장/퇴장 시 로비 집합 갱신
    uint32_t _next_room_id = 0;   // 마지막으로 발급한 방 번호(GameLoop 스레드 전용)
};
