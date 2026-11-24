#include "stdafx.h"
#include "Server.h"
#include "Session/SessionManager.h"
#include "GameLoop.h"
#include "Db/DbJobQueue.h"
#include "Db/DbResultQueue.h"
#include "Db/DbWorker.h"
#include "Redis/RedisWorker.h"
#include "Redis/UserCache.h"
#include "Data/DbWriteKinds.h"
#include "User/UserManager.h"
#include "Common/Log.h"
#include "Common/ServerConfig.h"
#include <algorithm>
#include <fstream>
#include <thread>

////////////////////////////////////////////////////////////////////////
// 서버 흐름 (스레드: I/O 풀 / GameLoop 1개 / DB 워커 풀)
// 1. 접속: Server::HandleAccept -> AdmitConnection(동시 접속 상한) -> Session::Start (I/O 스레드, 세션 strand)
// 2. 수신: Session::HandleReadBody -> MessageQueue::Push (헤더 6바이트 + protobuf body)
// 3. 처리: GameLoop::Tick -> 미등록 타입/허용 상태/요청 빈도 확인(PacketDispatcher 등록 정보) -> User/Room/LobbyManager 핸들러
// 4. DB: 핸들러가 DbJobQueue에 작업 -> DbWorker가 MySQL 처리 -> DbResultQueue -> 다음 틱에 GameLoop에서 콜백
//    Redis도 같은 방식(RedisJobQueue -> RedisWorker -> DbResultQueue)
// 5. 송신: Session::SendPacket -> 세션 strand에서 묶음 전송(전송 중이 아니면 즉시 1개)
////////////////////////////////////////////////////////////////////////
// 설정: 작업 폴더의 server.conf(형식/키는 server.conf.example). 없거나 잘못되면 이유를 출력하고 종료
// 실행: OZServer.exe [io_threads] [db_workers] [log_level] — 인자가 있으면 server.conf 값보다 우선
static constexpr const char* CONFIG_PATH = "server.conf";

int main(int argc, char* argv[]) 
{
    ServerConfig config;
    std::string config_error = "file not found (copy server.conf.example to server.conf)";
    std::ifstream config_file(CONFIG_PATH);
    if (!config_file || !ParseServerConfig(config_file, config, config_error)) {
        std::cerr << "Config error (" << CONFIG_PATH << "): " << config_error << std::endl;
        return 1;
    }
    try {
        if (argc > 1) config.io_threads = std::max(1u, static_cast<unsigned>(std::stoul(argv[1])));
        if (argc > 2) config.db.worker_count = std::max(1u, static_cast<unsigned>(std::stoul(argv[2])));
        if (argc > 3 && !ParseLogLevel(argv[3], config.log_level)) throw std::invalid_argument("log_level");
    }
    catch (const std::exception&) {
        std::cerr << "Usage: OZServer.exe [io_threads] [db_workers] [debug|info|warn|error]" << std::endl;
        return 1;
    }

    // 로그 스레드 시작. 어떤 경로로 main을 빠져나가도 남은 로그를 모두 쓰고 종료(가드 소멸자)
    // 파일: logs/server_YYYY-MM-DD.log(+ Warn 이상 error_YYYY-MM-DD.log), 보관 기간이 지난 날짜 파일은 자동 삭제
    GetLogger().SetSink(MakeConsoleFileSink("logs", config.log_retention_days));
    GetLogger().SetLevel(config.log_level);
    GetLogger().Start();
    struct LoggerStopGuard { ~LoggerStopGuard() { GetLogger().Stop(); } } logger_stop_guard;

    try {
        boost::asio::io_context io_context;

        // 포트를 가장 먼저 확보: 이미 사용 중이면(다른 서버 실행 중) DB에 연결하지 않고 바로 종료 -> 서버는 항상 하나만 동작.
        // accept 처리는 io_context.run() 이후에 시작되므로 DB 연결 전에 열어도 요청이 처리되지 않음
        LobbyManager lobby_manager;   // 로비 채널(GameLoop 스레드 전용). UserManager/RoomManager가 참조하므로 먼저 생성
        SessionManager session_manager;
        Server server(io_context, session_manager, lobby_manager);
        if (!server.Start(config.port)) {
            LogError("Server startup aborted: port ", config.port, " is already in use (another OZServer running?)");
            return 1;
        }

        // DB 워커 풀 시작 (워커마다 전용 스레드 + 전용 MySQL 커넥션, GameLoop 스레드는 절대 직접 쿼리하지 않음)
        // 클라이언트 접속(listen)을 받기 전에 먼저 연결하고, 실패하면 로그를 남기고 바로 종료
        DbJobQueue db_job_queue;
        DbResultQueue db_result_queue;
        const unsigned db_workers = config.db.worker_count;
        // 지연 쓰기 전용 워커 1개(같은 DB 설정, 커넥션 1개 추가): 워커가 하나라 넣은 순서대로 실행 -> 같은 대상의 묶음이 역전되지 않음
        DbJobQueue db_write_queue;
        DbConfig db_write_config = config.db;
        db_write_config.worker_count = 1;
        DbWorker db_worker(db_job_queue, db_result_queue, std::move(config.db));
        DbWorker db_write_worker(db_write_queue, db_result_queue, std::move(db_write_config));
        if (!db_worker.Start() || !db_write_worker.Start()) {
            db_worker.Stop();
            LogError("Server startup aborted: cannot connect to MySQL");
            return 1;
        }
        // Redis 워커(전용 스레드 + 커넥션 1개, 유저 정보 캐시). 연결 실패 시 종료
        RedisJobQueue redis_job_queue;
        RedisWorker redis_worker(redis_job_queue, std::move(config.redis));
        if (!redis_worker.Start()) {
            db_worker.Stop();
            db_write_worker.Stop();
            LogError("Server startup aborted: cannot connect to Redis");
            return 1;
        }
        // DB에 동시에 넘기는 로그인 상한 = 워커 수 × 8, 나머지는 로그인 대기열에서 대기.
        // 자리는 틱(50ms)마다 채워지므로 처리량 상한 = 상한 × 20/초. ×2(16)는 ~320건/초로 DB(~550건/초)보다 낮아 손해,
        // ×8(64)에서 대기열 없을 때와 같은 처리량(loadtest.md "로그인 대기열")
        UserCache user_cache(redis_job_queue, db_result_queue, std::chrono::seconds(config.user_cache_ttl_seconds));
        DbWriteBuffer db_write_buffer(db_write_queue, GetDbWriteKindSpecs());   // GameLoop 스레드 전용
        UserManager user_manager(db_job_queue, db_result_queue, lobby_manager, user_cache, db_write_buffer, db_workers * 8);

        LogInfo("Room Server started on port ", config.port, " (io_threads=", config.io_threads, ", db_workers=", db_workers,
                ", log_level=", LogLevelName(config.log_level), ")");

        // 단계적 종료(Ctrl+C / SIGTERM): 첫 신호 = 새 접속 차단 + GameLoop가 모든 요청 거부, 접수된 작업 처리 후 10초 뒤 세션 닫기.
        // 두 번째 신호 = 기다리지 않고 바로 세션 닫기. 신호 대기와 cancel은 같은 strand에서만(signal_set은 스레드 안전하지 않음).
        // 거부 단계 동안 다음 신호 대기가 걸려 있어 접속이 0이어도 io_context.run()이 반환되지 않음
        auto signal_strand = boost::asio::make_strand(io_context);
        boost::asio::signal_set signals(signal_strand, SIGINT, SIGTERM);

        // 게임 루프 시작. 종료 조건이 되면(GameLoop 스레드) 신호 strand로 넘겨 신호 대기를 풀고 세션을 닫음 -> run() 반환
        GameLoop game_loop(server.GetMessageQueue(), server.GetRoomManager(), lobby_manager, user_manager, session_manager,
                           db_result_queue, db_write_buffer, server.GetConnectionLimiter(), [&] {
                               boost::asio::post(signal_strand, [&] {
                                   boost::system::error_code ignored;
                                   signals.cancel(ignored);
                                   server.Stop();
                               });
                           });
        game_loop.Start();

        bool shutdown_started = false;   // signal_strand 전용
        std::function<void(const boost::system::error_code&, int)> on_signal =
            [&](const boost::system::error_code& error, int signal_number) {
                if (error) return;   // cancel(종료 조건 충족) -> operation_aborted
                if (shutdown_started) {
                    LogInfo("Second shutdown signal (", signal_number, "): closing immediately");
                    server.Stop();
                    return;
                }
                shutdown_started = true;
                LogInfo("Shutdown requested (signal ", signal_number, ")");
                server.StopAccepting();
                game_loop.BeginShutdown();
                signals.async_wait(on_signal);
            };
        signals.async_wait(on_signal);

        // I/O 스레드 풀: main 스레드 + (io_threads - 1)개. 모든 세션/acceptor가 닫혀 비동기 작업이 없어지면 run()이 반환됨
        std::vector<std::thread> io_pool;
        io_pool.reserve(config.io_threads - 1);
        for (unsigned i = 1; i < config.io_threads; ++i) {
            io_pool.emplace_back([&io_context] { io_context.run(); });
        }
        io_context.run();
        for (auto& t : io_pool) t.join();

        // 종료 순서: GameLoop 정지(+ 지연 쓰기 모두 내보냄) -> 남은 DB/Redis 작업 처리 후 워커 정지 -> 남은 이벤트 마지막 정리
        game_loop.Stop();
        db_worker.Stop();
        db_write_worker.Stop();
        redis_worker.Stop();
        game_loop.Flush();
        LogInfo("Server shutdown complete");
    }
    catch (std::exception& e) {
        LogError("Unhandled exception: ", e.what());
        return 1;
    }

    return 0;
}