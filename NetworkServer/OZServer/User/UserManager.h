#pragma once
#include "stdafx.h"
#include <chrono>
#include <cstdint>
#include <unordered_map>
#include <unordered_set>
#include "Common/Common.h"
#include "Db/DbJobQueue.h"
#include "Db/DbResultQueue.h"
#include "User/LoginQueue.h"
#include "Common/KeyedRateLimiter.h"
#include "Network/PacketPolicy.h"
#include "Lobby/LobbyManager.h"
#include "Redis/UserCache.h"
#include "Db/DbWriteBuffer.h"

// 로그인(ID만, 없으면 자동 생성) 처리. DB 조회/생성은 DbWorker 스레드에서, 상태 변경은 GameLoop 스레드에서만 수행.
// 로그인 순서: 유저 정보 캐시(Redis) 조회 -> 적중이면 DB 없이 완료 / 미적중·Redis 오류면 DB 조회(없으면 생성) 후 캐시 저장.
// login_date는 지연 쓰기 큐(DbWriteBuffer)로 모아서 갱신(신규 유저는 INSERT에 포함).
// 로그인 대기열: DB에 동시에 넘기는 로그인은 max_inflight개까지만, 나머지는 FIFO 대기열에서 순번 알림(S_LoginQueue)을 받으며 대기.
// 같은 ID 동시 접속: 먼저 로그인한 쪽 유지, 나중 요청은 ERR_ALREADY_LOGGED_IN(반복 시 ERR_RATE_LIMITED).
class UserManager {
public:
    UserManager(DbJobQueue& job_queue, DbResultQueue& result_queue, LobbyManager& lobby, UserCache& cache,
                DbWriteBuffer& write_buffer, uint32_t max_inflight);

    void HandleLogin(SessionPtr session, const protobuf::C_Login& request);
    void HandleLogout(SessionPtr session);
    // 연결 종료 정리(online 해제, 로비 채널 반환). 방 퇴장(RoomManager)과 순서 무관
    void HandleSessionDisconnected(SessionPtr session);
    // 전체 접속자 요약(/all): 로그인 인원 / 로비 인원 / 방 인원(유저별 목록 없음 — 응답 크기가 인원과 무관)
    void HandleOnlineList(SessionPtr session, const protobuf::C_UserList& request) const;

    // 종료 시작: 대기열(아직 DB로 넘기지 않은 요청)을 code로 모두 거부하고, 이후 캐시 미적중도 DB로 넘기지 않고 거부.
    // 이미 캐시 조회/DB 처리 중인 로그인은 끝까지 처리
    void BeginShutdown(protobuf::StatusCode code);
    // 캐시 조회 중이거나 DB 처리 중인 로그인이 남았는지(종료 대기 판정)
    bool HasPendingLogins() const { return !_pending_logins.empty(); }
    size_t GetPendingLoginCount() const { return _pending_logins.size(); }

    // 매 틱(DB 결과 콜백 다음) 호출: 빈 자리만큼 대기열에서 꺼내 DB로 넘기고, QUEUE_NOTICE_INTERVAL마다 순번 알림
    void Update();

    size_t   GetLoginWaitingCount() const { return _waiting.Size(); }
    uint32_t GetLoginInflightCount() const { return _inflight; }
    // 로그인 빈도 초과로 거부한 수를 가져가며 0으로 초기화([Stats] 출력용)
    uint64_t TakeLoginRateLimitedCount() { return std::exchange(_login_rate_limited, 0); }

private:
    // user.uuid 컬럼이 varchar(20)
    static constexpr size_t MAX_UUID_LENGTH = 20;
    // 신규 유저 자동 닉네임: 15자리 숫자(user.nickname varchar(20) 이내, UNIQUE). 사용 중이면 재생성
    static constexpr size_t GENERATED_NICKNAME_LENGTH = 15;
    static constexpr int    MAX_NICKNAME_ATTEMPTS = 5;
    // 로그인 대기열 상한. 넘으면 ERR_SERVER_BUSY로 즉시 거절(서버 메모리 보호)
    static constexpr size_t MAX_LOGIN_WAITING = 10000;
    static constexpr auto   QUEUE_NOTICE_INTERVAL = std::chrono::seconds(1);
    // 로그인 레이트리밋 키 정리 주기(다시 가득 찬 = 최근 쓰지 않은 IP/ID 항목 삭제)
    static constexpr auto   LIMITER_PRUNE_INTERVAL = std::chrono::seconds(60);

    struct WaitingLogin {
        SessionPtr  session;
        std::string uuid;
    };

    struct LoginRecord {
        protobuf::StatusCode code = protobuf::OK;
        UserRow              user;
        bool                 created = false;   // 이번에 생성(INSERT에 login_date 포함 -> 지연 쓰기 불필요)
    };

    void RemoveOnline(const SessionPtr& session);
    // IP별 시도 / ID별 실패 한도 확인. 넘었으면 ERR_RATE_LIMITED 응답 후 false
    bool CheckLoginRateLimit(const SessionPtr& session, const std::string& uuid, std::chrono::steady_clock::time_point now);
    // 캐시 조회 결과: 적중이면 완료, 아니면 DB로(빈 자리 있으면 바로, 없으면 대기열)
    void OnCacheResult(SessionPtr session, std::string uuid, std::optional<UserRow> cached);
    void StartLogin(SessionPtr session, std::string uuid);   // DB 작업 투입(_inflight 증가)
    void AdmitWaiting();
    void NotifyQueuePositions();
    void OnLoginResult(const SessionPtr& session, const std::string& uuid, LoginRecord record);
    // 로그인 마무리(캐시 적중 / DB 결과 공통): online 등록 + 로비 채널 배정 + S_Login. source는 로그 구분용(cache/db)
    void CompleteLogin(const SessionPtr& session, const std::string& uuid, int64_t user_id, std::string nickname,
                       const char* source);

    DbJobQueue&    _job_queue;
    DbResultQueue& _result_queue;
    LobbyManager&  _lobby;            // 로그인 성공 시 채널 배정, 로그아웃/종료 시 반환
    UserCache&     _cache;            // 로그인 시 유저 정보 조회/저장(Redis)
    DbWriteBuffer& _write_buffer;     // login_date 지연 쓰기

    // uuid -> 접속 중인 세션. GameLoop 스레드에서만 접근(락 불필요)
    std::unordered_map<std::string, SessionPtr> _online;
    // 캐시 조회 중이거나 대기열에 있거나 DB 처리 중인 uuid. 같은 ID 재요청은 즉시 ERR_ALREADY_LOGGED_IN ->
    // DB 워커가 여러 개여도 같은 uuid의 로그인 작업이 동시에 돌지 않음(GameLoop 스레드 전용)
    std::unordered_set<std::string> _pending_logins;

    LoginQueue<WaitingLogin> _waiting{ MAX_LOGIN_WAITING };
    const uint32_t _max_inflight;     // DB에 동시에 넘기는 로그인 수 상한(기본: DB 워커 수 × 8, main.cpp)
    uint32_t _inflight = 0;           // DB 처리 중인 로그인 수
    std::chrono::steady_clock::time_point _next_queue_notice{};
    protobuf::StatusCode _shutdown_code = protobuf::OK;   // OK가 아니면 종료 중(새 DB 작업 대신 이 코드로 거부)

    // 로그인 레이트리밋(GameLoop 스레드 전용). IP: 모든 시도(루프백 제외), ID: ERR_ALREADY_LOGGED_IN 실패만 셈
    KeyedRateLimiter _login_ip_limiter{ LOGIN_IP_RATE_LIMIT };
    KeyedRateLimiter _login_fail_limiter{ LOGIN_FAIL_RATE_LIMIT };
    std::chrono::steady_clock::time_point _next_limiter_prune{};
    uint64_t _login_rate_limited = 0;
};
