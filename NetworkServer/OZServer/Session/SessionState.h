#pragma once
#include <cstdint>

// 세션의 FSM 상태. 무거운 Session.h 없이도 쓸 수 있도록 분리(패킷 허용 규칙/테스트에서 사용)
enum class SessionState : uint8_t {
    Unauthenticated, // 연결 직후, 로그인 전 (기본값). 로그인 실패/로그아웃 시에도 이 상태
    LoggingIn,       // 로그인 요청 후 결과 대기(대기열 포함). 이 상태에서는 모든 요청 거부 -> 연결 1개당 로그인 요청 1건
    Lobby,           // 로그인 완료, 어떤 방에도 속하지 않음
    InRoom,          // 방에 참가한 상태(게임 대기/카운트다운/결과 포함)
    InGame,          // 방 게임 진행 중(이동 허용). 진행이 끝나면 InRoom — Room::UpdateGame이 바꿈
};
