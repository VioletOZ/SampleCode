#pragma once
#include <chrono>
#include <cstdint>
#include <initializer_list>
#include <string>
#include "Session/SessionState.h"
#include "Common/TokenBucket.h"
#include "proto/Common.pb.h"

// 패킷을 허용하는 세션 상태 집합(FSM 가드). 핸들러 등록 때 함께 지정한다(PacketDispatcher::Register).
// 등록하지 않은 타입은 모든 상태에서 거부(기본 거부). LoggingIn을 허용하는 패킷은 없으므로
// 로그인 결과가 나올 때까지 모든 요청(추가 C_Login 포함)이 ERR_INVALID_STATE로 거부된다.
class AllowedStates {
public:
    constexpr AllowedStates() = default;   // 아무 상태도 허용 안 함
    constexpr AllowedStates(std::initializer_list<SessionState> states)
    {
        for (SessionState state : states) _bits |= Bit(state);
    }
    constexpr bool Has(SessionState state) const { return (_bits & Bit(state)) != 0; }

private:
    static constexpr uint8_t Bit(SessionState state) { return static_cast<uint8_t>(1u << static_cast<uint8_t>(state)); }
    uint8_t _bits = 0;
};

// 프로토콜 위반(미등록 타입 / 파싱 실패) 누적 한도. 도달하면 에러 응답 후 연결을 끊는다.
constexpr uint32_t MAX_PROTOCOL_VIOLATIONS = 3;

inline bool IsProtocolViolationLimitReached(uint32_t violation_count)
{
    return violation_count >= MAX_PROTOCOL_VIOLATIONS;
}

// 세션별 미전송(대기 + 전송 중) 바이트 상한. 넘으면 못 따라오는 클라이언트로 보고 연결을 끊는다(서버 메모리 보호).
constexpr size_t MAX_SEND_QUEUE_BYTES = 1024 * 1024;

// 대기열이 비어 있으면 크기와 무관하게 1개는 허용(최대 크기 패킷도 항상 보낼 수 있도록)
inline bool IsSendQueueOverLimit(size_t pending_bytes, size_t incoming_bytes)
{
    return pending_bytes > 0 && pending_bytes + incoming_bytes > MAX_SEND_QUEUE_BYTES;
}

// 채팅 길이 상한: 한글 기준 50자(UTF-8 한글 1자 = 3바이트). 바이트로 판정 -> 영문/숫자는 150자
constexpr size_t MAX_CHAT_BYTES = 150;

inline bool IsValidChatMessage(const std::string& message)
{
    return !message.empty() && message.size() <= MAX_CHAT_BYTES;
}

// 연결 유지 점검(세션별 타이머, I/O 스레드의 세션 strand에서 CONNECTION_CHECK_INTERVAL마다)
// - 마지막으로 패킷을 끝까지 받은 지 IDLE_TIMEOUT 경과 -> 끊음(클라이언트는 HEARTBEAT_INTERVAL마다 C_Heartbeat 전송)
// - Unauthenticated 상태가 LOGIN_DEADLINE 이상 지속 -> 끊음(LoggingIn 대기열 대기는 제외)
constexpr auto CONNECTION_CHECK_INTERVAL = std::chrono::seconds(10);
constexpr auto HEARTBEAT_INTERVAL        = std::chrono::seconds(30);
constexpr auto IDLE_TIMEOUT              = std::chrono::seconds(60);
constexpr auto LOGIN_DEADLINE            = std::chrono::seconds(30);
// 종료 요청(Session::Disconnect) 후 남은 응답 전송을 기다리는 최대 시간. 넘으면 강제 종료(수신하지 않는 상대 대비)
constexpr auto CLOSE_WRITE_TIMEOUT       = std::chrono::seconds(5);

enum class ConnectionTimeout { None, Idle, Login };

// idle: 마지막 수신 후 경과 시간, unauthenticated: Unauthenticated 상태가 이어진 시간(다른 상태면 무시)
inline ConnectionTimeout CheckConnectionTimeout(SessionState state,
                                                std::chrono::steady_clock::duration idle,
                                                std::chrono::steady_clock::duration unauthenticated)
{
    if (idle >= IDLE_TIMEOUT) return ConnectionTimeout::Idle;
    if (state == SessionState::Unauthenticated && unauthenticated >= LOGIN_DEADLINE) return ConnectionTimeout::Login;
    return ConnectionTimeout::None;
}

// ---- 레이트리밋 / 남용 방지 (초과 응답은 ERR_RATE_LIMITED) ----
// 1단계 세션당 전체 패킷(C_Heartbeat 포함, I/O 스레드에서 판정): 초과분은 GameLoop로 넘기지 않고 버림.
//   연결 유지 점검 구간(CONNECTION_CHECK_INTERVAL) 동안 MAX_RATE_DROPS개를 넘게 버리면 연결 종료
constexpr RateLimit PACKET_RATE_LIMIT{ 40, 20 };
constexpr uint32_t  MAX_RATE_DROPS = 100;
// 2단계 요청 종류별(GameLoop): 핸들러 등록 때 RequestLimit을 지정하면 핸들러 호출 전에 세션의 그 종류 버킷을 확인하고,
//   초과면 핸들러를 부르지 않고 등록된 응답 타입으로 ERR_RATE_LIMITED(연결은 유지). 종류 추가 = enum 값 + 아래 표 한 줄
// 채팅: 1건이 방/로비 인원 수만큼 송신으로 불어남
constexpr RateLimit CHAT_RATE_LIMIT{ 5, 2 };
// 목록: 방 목록(ROOM_LIST)은 응답 크기가 방 수에 비례 -> 사람 입력 수준으로 제한(/all은 인원 요약이라 제외)
constexpr RateLimit LIST_RATE_LIMIT{ 3, 1 };
// 방 게임 이동(GAME_MOVE): 클라이언트는 초당 10회 — 여유를 두고 제한(이동 거리 검증은 Room/RoomGame.h의 이동 예산)
constexpr RateLimit MOVE_RATE_LIMIT{ 20, 15 };
enum class RequestLimit : uint8_t { None, Chat, List, Move, Count };
inline constexpr size_t REQUEST_LIMIT_COUNT = static_cast<size_t>(RequestLimit::Count);
// [RequestLimit] = 한도. None은 사용하지 않음(Session::TryTakeRequestToken이 항상 허용)
inline constexpr RateLimit REQUEST_RATE_LIMITS[REQUEST_LIMIT_COUNT] = { {}, CHAT_RATE_LIMIT, LIST_RATE_LIMIT, MOVE_RATE_LIMIT };
// 방 목록 한 페이지의 방 수(방 번호 순). 응답 크기가 방 수와 무관하게 고정
constexpr uint32_t  ROOM_LIST_PAGE_SIZE = 10;
// 3단계 동시 접속(accept): 서버 전체 / IP별(0 = IP별 제한 안 함). 초과 시 응답 없이 소켓을 닫음
// IP 기준 제한은 현재 비활성(사용자 결정: 기능만 두고 동작은 막음 — NAT/LB 환경, 클라이언트 쪽에서도 일부 처리 가능).
// 다시 켤 때 권장값: MAX_SESSIONS_PER_IP = 16, LOGIN_IP_RATE_LIMIT{ 10, 1 }
constexpr size_t MAX_SESSIONS        = 10000;
constexpr size_t MAX_SESSIONS_PER_IP = 0;
// 4단계 로그인(GameLoop): IP별 시도 수({0,0} = 제한 없음) / ID별 실패(ERR_ALREADY_LOGGED_IN) 수. 초과 시 S_Login{ERR_RATE_LIMITED}
constexpr RateLimit LOGIN_IP_RATE_LIMIT{ 0, 0 };
constexpr RateLimit LOGIN_FAIL_RATE_LIMIT{ 5, 0.1 };

// 접속한 클라이언트 주소(accept 시 1회 기록 후 불변)
struct ClientAddress {
    std::string ip;
    bool        loopback = false;
};

// IP 기준 제한(동시 접속 IP별, 로그인 IP별)에서 제외할 주소.
// 루프백: 부하 테스트(LoadTest)가 한 IP(127.0.0.1)에서 수천 개 접속하므로 제외. 서버 전체 상한과 세션/채팅/ID별 제한은 적용
inline bool IsIpLimitExempt(const ClientAddress& address)
{
    return address.loopback;
}
