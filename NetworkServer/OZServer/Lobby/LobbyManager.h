#pragma once
#include "stdafx.h"
#include <unordered_set>
#include <utility>
#include "Session/Session.h"
#include "Lobby/ChannelAllocator.h"
#include "proto/ChatPacket.pb.h"
#include "proto/LobbyPacket.pb.h"

// 로비 채널: 로그인한 유저를 정원 CHANNEL_CAPACITY의 채널로 나눠, 로비 채팅/로비 유저 목록을 같은 채널 안에서만 처리한다.
// (로비 전원 브로드캐스트는 인원² 비용 -> 로비 1000명에서 GameLoop가 멈췄던 문제 해결)
// 채널 소속은 로그인부터 로그아웃/연결 종료까지 유지되고, 방에 들어가 있는 동안에는 그 채널의 "로비 집합"에서만 빠진다.
// 모든 상태는 GameLoop 스레드 전용(락 없음).
class LobbyManager {
public:
    static constexpr uint32_t CHANNEL_CAPACITY = 100;

    LobbyManager() : _allocator(CHANNEL_CAPACITY) {}

    // 로그인 성공(-> Lobby): 채널 배정 + 로비 집합에 추가. 배정된 채널 번호 반환
    uint32_t Enter(const SessionPtr& session);
    // 방 입장: 로비 집합에서만 제거(채널 소속 유지)
    void EnterRoom(const SessionPtr& session);
    // 방 퇴장(-> Lobby): 로비 집합에 다시 추가. 이미 채널을 반환한 세션(로그아웃/종료 처리 후)은 무시
    void ReturnFromRoom(const SessionPtr& session);
    // 로그아웃/연결 종료: 로비 집합에서 제거 + 채널 반환. 여러 번 호출해도 안전
    void Exit(const SessionPtr& session);
    // 채널 이동(로비 상태에서만 호출 — FSM). 같은 채널 / 없는 채널 / 정원 초과면 해당 오류 코드, 성공 시 OK
    protobuf::StatusCode MoveChannel(const SessionPtr& session, uint32_t channel);

    // ---- 로비 요청 처리(로비 상태에서만 — 허용 상태는 GameLoop 등록) ----
    // 채널 이동(MoveChannel) + 결과 응답(새 채널 또는 그대로, 전체 채널 수)
    void HandleChannelMove(const SessionPtr& session, const protobuf::C_ChannelMove& request);
    // 로비 채팅: 같은 채널의 로비 유저에게만
    void HandleChatMessage(const SessionPtr& session, const protobuf::C_ChatPacket& request);
    // 로비 인원 목록(room_id 0 요청): 같은 채널에서 지금 로비에 있는 유저
    void HandleUserList(const SessionPtr& session);

    // 같은 채널의 로비 세션 전원에게 전송(직렬화 1회, 버퍼 공유)
    void Broadcast(uint32_t channel, std::string message) const;
    size_t GetChannelCount() const { return _allocator.GetChannelCount(); }
    // 지금 로비에 있는 인원(모든 채널의 로비 집합 크기 합, 방 안 인원 제외). 채널 수만큼 순회
    size_t GetLobbyUserCount() const
    {
        size_t count = 0;
        for (const LobbySet& lobby : _lobbies) count += lobby.size();
        return count;
    }

    using LobbySet = std::unordered_set<SessionPtr>;
    // 채널의 로비 세션 집합(읽기 전용, 복사 없음). 없는 채널이면 nullptr. GameLoop 스레드에서 그 호출 안에서만 사용
    const LobbySet* FindLobby(uint32_t channel) const;

private:
    LobbySet* FindMutableLobby(uint32_t channel) { return const_cast<LobbySet*>(FindLobby(channel)); }

    ChannelAllocator      _allocator;
    std::vector<LobbySet> _lobbies;   // [채널 - 1] = 그 채널에서 지금 로비에 있는 세션
};
