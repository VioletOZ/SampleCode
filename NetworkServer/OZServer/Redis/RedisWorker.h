#pragma once
#include <atomic>
#include <chrono>
#include <cstdint>
#include <memory>
#include <thread>
#include "Redis/RedisJobQueue.h"
#include "Redis/RedisConfig.h"

// Redis 워커: 전용 스레드 1개 + 커넥션 1개(DbWorker와 같은 구조, 커넥션을 스레드 간 공유하지 않음 -> 락 없음).
// GameLoop 스레드는 Redis를 직접 호출하지 않고 RedisJobQueue로 작업을 넘기며, 결과가 필요한 작업은
// 작업 안에서 DbResultQueue에 콜백을 넣어 다음 틱에 GameLoop 스레드에서 받는다.
// 실행 중 Redis 장애: 서버는 멈추지 않는다 — 실패한 작업은 on_failure 호출, 다음 명령에서 자동 재연결.
// 실패 후 RETRY_INTERVAL 동안은 Redis에 보내지 않고 바로 실패 처리(작업마다 시간 초과를 기다려 큐가 밀리지 않게).
class RedisWorker {
public:
    // 이 서버가 쓰는 키 접두사(다른 용도의 키와 구분)
    static constexpr const char* KEY_PREFIX = "oz:";
    // 실패 후 다시 Redis에 보내 보기까지의 간격(그 사이 작업은 바로 실패 처리)
    static constexpr auto RETRY_INTERVAL = std::chrono::seconds(1);

    RedisWorker(RedisJobQueue& job_queue, RedisConfig config);
    ~RedisWorker();   // sw::redis::Redis가 여기선 전방 선언 상태라 .cpp에서 정의

    // 연결 + PING 확인 후 스레드 시작. 실패하면 false(스레드 시작 안 함). 키는 지우지 않음(캐시는 재시작 후에도 유효)
    bool Start();
    // 정지 요청 후에도 큐에 남은 작업은 모두 처리하고 종료
    void Stop();

private:
    void Run();
    void Execute(RedisJob& job);

    RedisJobQueue& _job_queue;
    RedisConfig    _config;

    std::unique_ptr<sw::redis::Redis> _redis;
    std::atomic<bool>                 _running;
    std::thread                       _thread;

    // 워커 스레드 전용: 연속 실패 수(0이 아니면 장애 중 — 첫 실패와 복구 때만 로그), 다음 시도 가능 시각
    uint64_t _consecutive_failures = 0;
    std::chrono::steady_clock::time_point _retry_after{};
};
