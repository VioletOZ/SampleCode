#pragma once
#include <thread>
#include <array>
#include <atomic>
#include <chrono>
#include <functional>
#include <iomanip>
#include <utility>
#include <unordered_map>
#include <unordered_set>
#include "Message/MessageQueue.h"
#include "Room/RoomManager.h"
#include "User/UserManager.h"
#include "Session/SessionManager.h"
#include "Network/PacketDispatcher.h"
#include "Network/PacketPolicy.h"
#include "Network/ConnectionLimiter.h"
#include "Db/DbResultQueue.h"
#include "Db/DbWriteBuffer.h"
#include "Common/Log.h"
#include "Common/TickStats.h"
#include "Common/ShutdownPolicy.h"

class GameLoop {
public:
    static constexpr int TPS = 20;
    static constexpr auto TICK_DURATION = std::chrono::milliseconds(1000 / TPS);
    static constexpr auto STATS_INTERVAL = std::chrono::seconds(60);  // [Stats] 출력 주기
    // 이보다 많이 밀리면 밀린 틱을 몰아서 실행하지 않고 건너뜀(게임 시간이 빨리 흐르는 것 방지). 5틱 = 250ms
    static constexpr auto MAX_TICK_LAG = TICK_DURATION * 5;

    GameLoop(MessageQueue& queue, RoomManager& room_manager, LobbyManager& lobby_manager, UserManager& user_manager,
             SessionManager& session_manager, DbResultQueue& db_result_queue, DbWriteBuffer& db_write_buffer,
             ConnectionLimiter& connection_limiter, std::function<void()> on_shutdown_ready)
        : _queue(queue), _room_manager(room_manager), _lobby_manager(lobby_manager), _user_manager(user_manager),
          _session_manager(session_manager), _db_result_queue(db_result_queue), _db_write_buffer(db_write_buffer),
          _connection_limiter(connection_limiter),
          _on_shutdown_ready(std::move(on_shutdown_ready)), _running(false), _tick_stats(TICK_DURATION) {
        RegisterHandlers();
    }

    void Start() {
        _running = true;
        _thread = std::thread(&GameLoop::Run, this);
    }

    // 단계적 종료 시작(신호 처리기 등 아무 스레드). 다음 틱부터 모든 요청을 ERR_SERVER_SHUTTING_DOWN으로 거부하고
    // 접수된 작업이 끝나고 SHUTDOWN_GRACE가 지나면 on_shutdown_ready를 1회 호출(GameLoop 스레드) -> main이 세션을 닫음
    void BeginShutdown() { _shutdown_requested = true; }

    // 루프 정지 후 지연 쓰기를 모두 내보냄 -> 이어지는 DB 워커 Stop()이 처리(정상 종료 시 유실 없음)
    void Stop() {
        _running = false;
        if (_thread.joinable()) _thread.join();
        _db_write_buffer.FlushAll();
    }

    // 종료 시 Stop() 이후 호출: 큐에 남은 disconnect 이벤트/DB 결과를 마지막으로 1회 처리
    // (루프 스레드가 join된 뒤 호출해야 하므로 단일 스레드에서 안전)
    void Flush() { Tick(); }

private:
    // 패킷 1종 = Register 1줄: 요청/응답 타입, 받는 세션 상태(기본 거부), 핸들러, 빈도 제한(선택).
    // 핸들러는 그 상태를 가진 Manager로 바로 보냄(방 = RoomManager, 로비 채널 = LobbyManager, 로그인 = UserManager)
    void RegisterHandlers() {
        using namespace protobuf;
        using enum SessionState;
        _dispatcher.Register<C_Login, S_Login>(USER_LOGIN, { Unauthenticated },
            [this](SessionPtr s, const C_Login& p) { _user_manager.HandleLogin(s, p); });
        _dispatcher.Register<C_Logout, S_Logout>(USER_LOGOUT, { Lobby, InRoom, InGame },
            [this](SessionPtr s, const C_Logout&) {
                _room_manager.LeaveCurrentRoom(s);  // 방에 있었다면 먼저 퇴장
                _user_manager.HandleLogout(s);
            });
        _dispatcher.Register<C_UserList, S_UserList>(USER_LIST, { Lobby, InRoom, InGame },
            [this](SessionPtr s, const C_UserList& p) { _user_manager.HandleOnlineList(s, p); });

        _dispatcher.Register<C_RoomCreate, S_RoomCreate>(ROOM_CREATE, { Lobby },
            [this](SessionPtr s, const C_RoomCreate& p) { _room_manager.HandleCreateRoom(s, p); });
        _dispatcher.Register<C_RoomJoin, S_RoomJoin>(ROOM_JOIN, { Lobby },
            [this](SessionPtr s, const C_RoomJoin& p) { _room_manager.HandleJoinRoom(s, p); });
        _dispatcher.Register<C_RoomLeave, S_RoomLeave>(ROOM_LEAVE, { InRoom, InGame },
            [this](SessionPtr s, const C_RoomLeave& p) { _room_manager.HandleLeaveRoom(s, p); });
        _dispatcher.Register<C_RoomList, S_RoomList>(ROOM_LIST, { Lobby },
            [this](SessionPtr s, const C_RoomList& p) { _room_manager.HandleRoomList(s, p); }, RequestLimit::List);

        // 방 안이면 방, 로비면 같은 채널
        _dispatcher.Register<C_ChatPacket, S_ChatPacket>(CHAT_MESSAGE, { Lobby, InRoom, InGame },
            [this](SessionPtr s, const C_ChatPacket& p) {
                if (s->GetState() == Lobby) _lobby_manager.HandleChatMessage(s, p);
                else _room_manager.HandleChatMessage(s, p);
            }, RequestLimit::Chat);
        // room_id 지정 또는 방 안이면 방 인원, 로비에서 0이면 같은 채널의 로비 인원
        _dispatcher.Register<C_RoomInfo, S_RoomInfo>(ROOM_USER_LIST, { Lobby, InRoom, InGame },
            [this](SessionPtr s, const C_RoomInfo& p) {
                if (p.room_info().room_id() == 0 && s->GetState() == Lobby) _lobby_manager.HandleUserList(s);
                else _room_manager.HandleUserList(s, p);
            });
        _dispatcher.Register<C_ChannelMove, S_ChannelMove>(LOBBY_CHANNEL_MOVE, { Lobby },   // 방 안에서는 나간 뒤 이동
            [this](SessionPtr s, const C_ChannelMove& p) { _lobby_manager.HandleChannelMove(s, p); });

        // 방 게임: 준비는 방 안(대기·카운트다운 — 단계 확인은 RoomGame), 이동은 진행 중(InGame)에만
        _dispatcher.Register<C_GameReady, S_GameReady>(GAME_READY, { InRoom },
            [this](SessionPtr s, const C_GameReady& p) { _room_manager.HandleGameReady(s, p); });
        _dispatcher.Register<C_GameMove, S_GameMove>(GAME_MOVE, { InGame },
            [this](SessionPtr s, const C_GameMove& p) { _room_manager.HandleGameMove(s, p); }, RequestLimit::Move);
    }

    void Run() {
        using Clock = std::chrono::steady_clock;
        auto next_tick = Clock::now();
        auto next_report = next_tick + STATS_INTERVAL;

        while (_running) {
            auto now = Clock::now();
            if (now >= next_tick) {
                const uint32_t processed = Tick();
                const auto done = Clock::now();
                _tick_stats.Record(std::chrono::duration_cast<std::chrono::microseconds>(done - now), processed);

                const TickSchedule schedule = NextTickTime(next_tick, done, TICK_DURATION, MAX_TICK_LAG);
                if (schedule.skipped > 0) OnTicksSkipped(schedule.skipped);
                next_tick = schedule.next;

                if (now >= next_report) {
                    ReportStats();
                    next_report = now + STATS_INTERVAL;   // 정지 후에도 보고를 몰아서 찍지 않음
                }
            }
            else {
                std::this_thread::sleep_until(next_tick);
            }
        }
    }

    // 서버 쪽 정지(과부하, 디버거 정지 등)는 운영자가 알아야 하므로 Warn. 단 보고 구간마다 첫 발생만 출력
    void OnTicksSkipped(uint32_t skipped) {
        _tick_stats.RecordSkipped(skipped);
        if (_skip_warned) return;
        _skip_warned = true;
        LogWarn("GameLoop fell behind, skipped ", skipped, " ticks (further skips this interval are counted in [Stats])");
    }

    // 처리한 메시지(패킷 + 연결 종료 이벤트) 수를 반환 — 틱 지표용
    // 메시지 1개 / DB 콜백 1개 단위로 예외를 격리(하나가 실패해도 다른 세션 처리와 서버는 계속)
    uint32_t Tick() {
        if (_shutdown_requested && !_shutting_down) StartShutdown();
        auto messages = _queue.PopAll();
        const uint32_t processed = static_cast<uint32_t>(messages.size());
        while (!messages.empty()) {
            const IncomingMessage& item = messages.front();
            RunGuarded([&] { ProcessMessage(item); },
                       " (session=", item.session->GetSessionId(), " type=", item.packet.type, item.is_disconnect ? " disconnect" : "", ")");
            messages.pop();
        }

        auto db_results = _db_result_queue.PopAll();
        while (!db_results.empty()) {
            RunGuarded([&] { db_results.front().callback(); }, " (DB callback)");
            db_results.pop();
        }

        const auto now = std::chrono::steady_clock::now();
        _user_manager.Update();          // DB 결과로 빈 자리만큼 로그인 대기열 진행
        _room_manager.Update(now);       // 방 게임 단계 전환·스냅샷
        _db_write_buffer.Update(now);    // 주기가 지난 지연 쓰기 내보냄
        if (_shutting_down) UpdateShutdown();
        return processed;
    }

    void StartShutdown() {
        _shutting_down = true;
        _shutdown_started = std::chrono::steady_clock::now();
        _user_manager.BeginShutdown(protobuf::ERR_SERVER_SHUTTING_DOWN);   // 로그인 대기열은 바로 거부
        LogInfo("Shutdown: rejecting all requests, finishing work in progress (logins in progress=",
                _user_manager.GetPendingLoginCount(), ")");
    }

    // 종료 중 매 틱: 지연 쓰기는 모으지 않고 바로 내보냄, 종료 조건(ShouldFinishShutdown)이 되면 1회 알림
    void UpdateShutdown() {
        _db_write_buffer.FlushAll();
        if (_shutdown_ready_sent) return;
        const auto elapsed = std::chrono::steady_clock::now() - _shutdown_started;
        if (!ShouldFinishShutdown(elapsed, _user_manager.HasPendingLogins())) return;

        _shutdown_ready_sent = true;
        if (_user_manager.HasPendingLogins()) {
            LogError("Shutdown: ", _user_manager.GetPendingLoginCount(), " logins still in progress after ",
                     SHUTDOWN_MAX_WAIT.count(), "s, closing anyway");
        }
        LogInfo("Shutdown: work finished, closing all sessions");
        if (_on_shutdown_ready) _on_shutdown_ready();
    }

    // 처리 순서: 연결 종료 정리 -> 미등록 타입(프로토콜 오류) -> 종료 중 거부 -> FSM 허용 확인(ERR_INVALID_STATE)
    //          -> 요청 빈도(ERR_RATE_LIMITED) -> 핸들러
    void ProcessMessage(const IncomingMessage& item) {
        const SessionPtr& session = item.session;
        if (item.is_disconnect) {
            _user_manager.HandleSessionDisconnected(session);
            _room_manager.LeaveCurrentRoom(session);   // 마지막 인원이면 방 삭제
            _session_manager.RemoveSession(session);
            return;
        }

        const protobuf::PacketType type = static_cast<protobuf::PacketType>(item.packet.type);
        if (!_dispatcher.IsRegistered(type)) {
            HandleProtocolError(session, item.packet.type, DispatchResult::UnknownType);
            return;
        }
        if (_shutting_down) {   // 종료 중: 등록된 모든 요청을 처리하지 않고 거부(응답 타입은 등록 정보로 생성)
            session->SendPacket(_dispatcher.SerializeStatus(type, protobuf::ERR_SERVER_SHUTTING_DOWN));
            return;
        }
        if (!_dispatcher.IsAllowed(session->GetState(), type)) {
            // 클라이언트가 유발할 수 있는 출력 -> Debug(응답으로 이미 알림)
            LogDebug("[FSM] state=", static_cast<int>(session->GetState()),
                     " 에서 type=", static_cast<uint16_t>(type), " 허용 안 됨 (ERR_INVALID_STATE 응답)");
            session->SendPacket(_dispatcher.SerializeStatus(type, protobuf::ERR_INVALID_STATE));
            return;
        }
        // 등록 때 지정한 요청 종류별 빈도 제한: 초과면 핸들러를 부르지 않고 ERR_RATE_LIMITED(연결 유지)
        const RequestLimit limit = _dispatcher.GetRequestLimit(type);
        if (!session->TryTakeRequestToken(limit, std::chrono::steady_clock::now())) {
            ++_request_rate_limited[static_cast<size_t>(limit)];
            session->SendPacket(_dispatcher.SerializeStatus(type, protobuf::ERR_RATE_LIMITED));
            return;
        }

        const DispatchResult result = _dispatcher.Dispatch(session, type, item.packet.body);
        if (result != DispatchResult::Ok) {
            HandleProtocolError(session, item.packet.type, result);
        }
    }

    // 예외가 나면 Error 로그(context는 로그 뒤에 붙임) 후 계속. 처리 중이던 요청의 상태 변경은 일부만 반영됐을 수 있음
    template<typename Func, typename... Context>
    void RunGuarded(Func&& func, const Context&... context) {
        try {
            func();
        }
        catch (const std::exception& e) {
            LogError("GameLoop exception: ", e.what(), context...);
        }
        catch (...) {
            LogError("GameLoop unknown exception", context...);
        }
    }

    // STATS_INTERVAL마다 1줄: 틱 처리 시간(평균/최대), 예산 초과 틱 수, 초당 처리 메시지, 접속 세션 수, 로그인 대기/처리 중 수,
    // 구간 동안의 레이트리밋 집계(접속 거부, 버린 패킷, 빈도 초과로 끊은 세션, 채팅/목록/로그인 거부), 지연 쓰기 묶음/행 수
    void ReportStats() {
        const TickSummary s = _tick_stats.TakeSummary();
        const ConnectionLimitCounters rl = _connection_limiter.TakeCounters();
        const DbWriteStats db_write = _db_write_buffer.TakeStats();
        _skip_warned = false;
        const double seconds = std::chrono::duration<double>(STATS_INTERVAL).count();
        LogInfo(std::fixed, std::setprecision(2),
                "[Stats] ticks=", s.ticks, " avg=", s.avg_ms, "ms max=", s.max_ms, "ms",
                " overrun=", s.overruns, " skipped=", s.skipped, " msg/s=", static_cast<uint64_t>(s.messages / seconds),
                " max_msg/tick=", s.max_messages, " sessions=", _session_manager.GetSessionCount(),
                " lobby_channels=", _lobby_manager.GetChannelCount(),
                " login_wait=", _user_manager.GetLoginWaitingCount(), " login_inflight=", _user_manager.GetLoginInflightCount(),
                " conn_reject=", rl.rejected, " rl_pkt_drop=", rl.packet_dropped, " rl_disc=", rl.rate_disconnected,
                " rl_chat=", TakeRequestRateLimited(RequestLimit::Chat), " rl_list=", TakeRequestRateLimited(RequestLimit::List),
                " rl_move=", TakeRequestRateLimited(RequestLimit::Move),
                " rl_login=", _user_manager.TakeLoginRateLimitedCount(),
                " db_write=", db_write.batches, "/", db_write.rows);
    }

    // 요청 종류별 빈도 초과 거부 수를 가져가며 0으로 초기화([Stats] 출력용)
    uint64_t TakeRequestRateLimited(RequestLimit limit) {
        return std::exchange(_request_rate_limited[static_cast<size_t>(limit)], 0);
    }

    // 미등록 타입 / 파싱 실패: 공용 에러 응답(S_ProtocolError) 후 위반 횟수 누적, 한도 도달 시 연결 종료
    void HandleProtocolError(const SessionPtr& session, uint16_t raw_type, DispatchResult result) {
        const protobuf::StatusCode code = (result == DispatchResult::UnknownType)
            ? protobuf::ERR_UNKNOWN_PACKET : protobuf::ERR_MALFORMED_PACKET;
        const uint32_t violations = session->AddProtocolViolation();

        LogDebug("[Protocol] session=", session->GetSessionId(), " type=", raw_type,
                 " ", protobuf::StatusCode_Name(code), " (", violations, "/", MAX_PROTOCOL_VIOLATIONS, ")");

        protobuf::S_ProtocolError response;
        response.set_status_code(code);
        response.set_request_type(raw_type);
        session->SendPacket(PacketUtils::Serialize(protobuf::PROTOCOL_ERROR, response));

        if (IsProtocolViolationLimitReached(violations)) {
            LogWarn("Protocol violation limit reached, disconnecting session ", session->GetSessionId());
            session->Disconnect();
        }
    }

    MessageQueue&     _queue;
    RoomManager&      _room_manager;
    LobbyManager&     _lobby_manager;
    UserManager&      _user_manager;
    SessionManager&   _session_manager;
    DbResultQueue&    _db_result_queue;
    DbWriteBuffer&    _db_write_buffer;     // GameLoop 스레드 전용(Stop 이후 FlushAll은 루프 스레드 join 뒤)
    ConnectionLimiter& _connection_limiter;
    std::function<void()> _on_shutdown_ready;   // 종료 조건 충족 시 1회(GameLoop 스레드)
    PacketDispatcher  _dispatcher;

    std::atomic<bool> _running;
    std::thread       _thread;
    TickStats         _tick_stats;   // GameLoop 스레드 전용
    bool              _skip_warned = false;   // 이번 보고 구간에 틱 건너뜀 경고를 출력했는지(GameLoop 스레드 전용)
    // 요청 종류별 빈도 초과로 거부한 수([RequestLimit], GameLoop 스레드 전용, [Stats] 출력 후 0)
    std::array<uint64_t, REQUEST_LIMIT_COUNT> _request_rate_limited{};
    std::atomic<bool> _shutdown_requested{ false };   // BeginShutdown(아무 스레드) -> 다음 틱에 StartShutdown
    bool              _shutting_down = false;         // 이하 GameLoop 스레드 전용
    bool              _shutdown_ready_sent = false;
    std::chrono::steady_clock::time_point _shutdown_started{};
};
