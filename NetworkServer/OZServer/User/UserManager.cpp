#include "stdafx.h"
#include "User/UserManager.h"
#include "Session/Session.h"
#include "Common/ServerCommon.h"
#include "Common/Log.h"
#include "Data/UserTable.h"

namespace {

// 로그인 중(LoggingIn)이던 요청이 실패: 다시 로그인할 수 있도록 Unauthenticated로 되돌리고 실패 응답
void FailLogin(const SessionPtr& session, protobuf::StatusCode code)
{
    session->SetState(SessionState::Unauthenticated);
    SendStatus<protobuf::S_Login>(session, protobuf::USER_LOGIN, code);
}

void SendQueuePosition(const SessionPtr& session, uint32_t position, uint32_t waiting)
{
    protobuf::S_LoginQueue notice;
    notice.set_position(position);
    notice.set_waiting(waiting);
    session->SendPacket(PacketUtils::Serialize(protobuf::LOGIN_QUEUE, notice));
}

}

UserManager::UserManager(DbJobQueue& job_queue, DbResultQueue& result_queue, LobbyManager& lobby, UserCache& cache,
                         DbWriteBuffer& write_buffer, uint32_t max_inflight)
    : _job_queue(job_queue), _result_queue(result_queue), _lobby(lobby), _cache(cache), _write_buffer(write_buffer),
      _max_inflight(std::max(1u, max_inflight))
{
}

bool UserManager::CheckLoginRateLimit(const SessionPtr& session, const std::string& uuid, std::chrono::steady_clock::time_point now)
{
    const ClientAddress& address = session->GetClientAddress();
    const bool ip_ok = IsIpLimitExempt(address) || _login_ip_limiter.TryTake(address.ip, now);
    if (ip_ok && _login_fail_limiter.HasToken(uuid, now)) return true;

    ++_login_rate_limited;
    SendStatus<protobuf::S_Login>(session, protobuf::USER_LOGIN, protobuf::ERR_RATE_LIMITED);
    return false;
}

void UserManager::HandleLogin(SessionPtr session, const protobuf::C_Login& request)
{
    const std::string& uuid = request.uuid();
    const auto now = std::chrono::steady_clock::now();
    if (!CheckLoginRateLimit(session, uuid, now)) return;   // DB 작업 전에 차단
    if (uuid.empty() || uuid.size() > MAX_UUID_LENGTH) {
        SendStatus<protobuf::S_Login>(session, protobuf::USER_LOGIN, protobuf::ERR_INVALID_ARGUMENT);
        return;
    }
    if (_online.count(uuid) || _pending_logins.count(uuid)) {
        _login_fail_limiter.TryTake(uuid, now);   // ID별 실패 누적(한도 소진 시 이 ID는 잠시 ERR_RATE_LIMITED)
        SendStatus<protobuf::S_Login>(session, protobuf::USER_LOGIN, protobuf::ERR_ALREADY_LOGGED_IN);
        return;
    }

    // 접수되면 LoggingIn: 결과가 나올 때까지 이 세션의 추가 요청(C_Login 포함)은 FSM에서 거부됨.
    // _pending_logins는 캐시 적중 / 대기열에서 끊김 / DB 결과(OnLoginResult) 중 하나에서 제거
    _pending_logins.insert(uuid);
    session->SetState(SessionState::LoggingIn);
    _cache.Get(uuid, [this, session = std::move(session), uuid](std::optional<UserRow> cached) mutable {
        OnCacheResult(std::move(session), std::move(uuid), std::move(cached));
    });
}

void UserManager::OnCacheResult(SessionPtr session, std::string uuid, std::optional<UserRow> cached)
{
    if (session->IsDisconnected()) {   // online 등록 전이라 끊김 처리에서 정리할 것은 없음
        _pending_logins.erase(uuid);
        return;
    }
    if (cached) {
        _pending_logins.erase(uuid);
        _write_buffer.Put(WriteKind::LoginDate, cached->id);
        CompleteLogin(session, uuid, cached->id, std::move(cached->nickname), "cache");
        return;
    }

    if (_shutdown_code != protobuf::OK) {   // 종료 중: 새 DB 작업을 만들지 않음
        _pending_logins.erase(uuid);
        FailLogin(session, _shutdown_code);
        return;
    }

    // 미적중 / Redis 오류: 평상시(빈 자리 있고 대기자 없음)에는 바로 DB로 -> 지연 증가 없음. 몰릴 때만 대기열(FIFO)
    if (_inflight < _max_inflight && _waiting.Empty()) {
        StartLogin(std::move(session), std::move(uuid));
        return;
    }
    if (!_waiting.TryPush({ session, uuid })) {
        _pending_logins.erase(uuid);
        FailLogin(session, protobuf::ERR_SERVER_BUSY);
        return;
    }
    const uint32_t waiting = static_cast<uint32_t>(_waiting.Size());
    SendQueuePosition(session, waiting, waiting);
}

void UserManager::Update()
{
    AdmitWaiting();

    const auto now = std::chrono::steady_clock::now();
    if (now >= _next_limiter_prune) {
        _next_limiter_prune = now + LIMITER_PRUNE_INTERVAL;
        _login_ip_limiter.Prune(now);
        _login_fail_limiter.Prune(now);
    }
    if (_waiting.Empty() || now < _next_queue_notice) return;
    _next_queue_notice = now + QUEUE_NOTICE_INTERVAL;
    NotifyQueuePositions();
}

void UserManager::BeginShutdown(protobuf::StatusCode code)
{
    _shutdown_code = code;
    WaitingLogin entry;
    while (_waiting.TryPop(entry)) {
        _pending_logins.erase(entry.uuid);
        if (!entry.session->IsDisconnected()) FailLogin(entry.session, code);
    }
}

// 빈 자리만큼 대기열 앞에서 꺼내 DB로 넘김. 대기 중에 끊긴 세션은 DB 작업 없이 버림
void UserManager::AdmitWaiting()
{
    WaitingLogin entry;
    while (_inflight < _max_inflight && _waiting.TryPop(entry)) {
        if (entry.session->IsDisconnected()) {
            _pending_logins.erase(entry.uuid);
            continue;
        }
        StartLogin(std::move(entry.session), std::move(entry.uuid));
    }
}

// 끊긴 대기자를 정리하고(재접속 시 같은 ID로 바로 다시 줄 설 수 있게) 남은 대기자에게 새 순번 알림
void UserManager::NotifyQueuePositions()
{
    _waiting.PruneAndNumber(
        [](const WaitingLogin& e) { return !e.session->IsDisconnected(); },
        [this](const WaitingLogin& e) { _pending_logins.erase(e.uuid); },
        [](const WaitingLogin& e, uint32_t position, uint32_t waiting) { SendQueuePosition(e.session, position, waiting); });
}

void UserManager::StartLogin(SessionPtr session, std::string uuid)
{
    ++_inflight;   // OnLoginResult에서 감소

    // 워커 스레드에서 실행: 조회 후 없으면 생성. 결과만 GameLoop 스레드로 넘김(기존 유저의 login_date는 지연 쓰기 큐).
    // 쿼리는 UserTable(DAO), 닉네임 생성 규칙은 여기
    _job_queue.Push({
        [uuid = std::move(uuid), session = std::move(session), this](mysqlx::Session& sess) {
            LoginRecord record;
            try {
                UserTable users(sess);
                if (std::optional<UserRow> found = users.FindByUuid(uuid)) {
                    record.user = std::move(*found);
                }
                else {
                    // 닉네임은 uuid(로그인 ID)와 무관한 랜덤 숫자로 생성 — 다른 유저 닉네임과의 충돌 방지 + uuid 비노출
                    std::string nickname;
                    for (int attempt = 0; attempt < MAX_NICKNAME_ATTEMPTS && nickname.empty(); ++attempt) {
                        std::string candidate = GenerateNumericString(GENERATED_NICKNAME_LENGTH);
                        if (!users.NicknameExists(candidate)) nickname = std::move(candidate);
                    }
                    if (nickname.empty()) {
                        LogError("UserManager: failed to generate a unique nickname for uuid=", uuid);
                        record.code = protobuf::ERR_UNKNOWN;
                    }
                    else {
                        record.user = users.Insert(uuid, std::move(nickname));
                        record.created = true;
                    }
                }
            }
            catch (const std::exception& e) {   // mysqlx::Error 포함 — 어떤 예외든 실패 결과를 돌려줘 세션이 LoggingIn에 멈추지 않게
                LogError("UserManager: login query failed: ", e.what());
                record = LoginRecord{};
                record.code = protobuf::ERR_UNKNOWN;
            }

            _result_queue.Push({
                [this, session, uuid, record = std::move(record)]() mutable {
                    OnLoginResult(session, uuid, std::move(record));
                }
            });
        }
    });
}

void UserManager::OnLoginResult(const SessionPtr& session, const std::string& uuid, LoginRecord record)
{
    --_inflight;
    _pending_logins.erase(uuid);
    if (session->IsDisconnected()) return;

    if (record.code != protobuf::OK) {
        FailLogin(session, record.code);
        return;
    }

    _cache.Set(uuid, record.user.id, record.user.nickname);   // 다음 로그인부터 DB 조회 생략
    if (!record.created) _write_buffer.Put(WriteKind::LoginDate, record.user.id);
    CompleteLogin(session, uuid, record.user.id, std::move(record.user.nickname), "db");
}

void UserManager::CompleteLogin(const SessionPtr& session, const std::string& uuid, int64_t user_id, std::string nickname,
                                const char* source)
{
    // 방어적 최종 판정: LoggingIn 중에는 다른 요청이 거부되고 같은 uuid는 _pending_logins로 막히므로 정상 흐름에서는 걸리지 않음
    if (session->GetState() != SessionState::LoggingIn) {
        SendStatus<protobuf::S_Login>(session, protobuf::USER_LOGIN, protobuf::ERR_INVALID_STATE);
        return;
    }
    if (_online.count(uuid)) {
        _login_fail_limiter.TryTake(uuid, std::chrono::steady_clock::now());
        FailLogin(session, protobuf::ERR_ALREADY_LOGGED_IN);
        return;
    }

    _online.emplace(uuid, session);
    session->SetUserId(uuid);
    session->SetUserName(std::move(nickname));
    session->SetState(SessionState::Lobby);
    const uint32_t channel = _lobby.Enter(session);

    protobuf::S_Login response;
    response.set_status_code(protobuf::OK);
    response.set_channel(channel);
    response.mutable_user_info()->set_id(std::to_string(user_id));
    response.mutable_user_info()->set_name(session->GetUserName());
    session->SendPacket(PacketUtils::Serialize(protobuf::USER_LOGIN, response));
    LogDebug("Login: uuid=", uuid, " nickname=", session->GetUserName(), " (", source, ")");
}

void UserManager::HandleLogout(SessionPtr session)
{
    // 방 퇴장은 호출 측(GameLoop)이 먼저 처리. 연결은 유지하므로 다른 ID로 다시 로그인 가능
    LogDebug("Logout: uuid=", session->GetUserId());
    RemoveOnline(session);
    _lobby.Exit(session);
    session->SetUserId("");
    session->SetUserName("");
    session->SetState(SessionState::Unauthenticated);

    SendStatus<protobuf::S_Logout>(session, protobuf::USER_LOGOUT, protobuf::OK);
}

void UserManager::HandleOnlineList(SessionPtr session, const protobuf::C_UserList& request) const
{
    // 로그인한 유저는 로비 아니면 방 -> 방 인원 = 전체 - 로비(유저 순회 없음, 채널 수만큼만 합산)
    const uint32_t total = static_cast<uint32_t>(_online.size());
    const uint32_t lobby = static_cast<uint32_t>(_lobby.GetLobbyUserCount());

    protobuf::S_UserList response;
    response.set_total(total);
    response.set_lobby(lobby);
    response.set_in_room(total - lobby);
    response.set_status_code(protobuf::OK);
    session->SendPacket(PacketUtils::Serialize(protobuf::USER_LIST, response));
}

void UserManager::RemoveOnline(const SessionPtr& session)
{
    auto it = _online.find(session->GetUserId());
    if (it != _online.end() && it->second == session) {
        _online.erase(it);
    }
}

void UserManager::HandleSessionDisconnected(SessionPtr session)
{
    RemoveOnline(session);
    _lobby.Exit(session);   // RoomManager의 방 퇴장보다 먼저 호출되어도 안전(채널 0이면 로비 집합에 다시 넣지 않음)
}
