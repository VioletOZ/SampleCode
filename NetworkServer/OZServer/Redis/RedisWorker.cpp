#include "stdafx.h"
#include "Redis/RedisWorker.h"
#include <sw/redis++/redis++.h>
#include "Common/Log.h"

namespace {
// 연결/응답 대기 상한. Redis가 멈춰도 작업 1건이 이 시간 안에 실패로 끝남(로그인이 오래 멈추지 않게)
constexpr auto REDIS_TIMEOUT = std::chrono::milliseconds(1000);
}

RedisWorker::RedisWorker(RedisJobQueue& job_queue, RedisConfig config)
    : _job_queue(job_queue), _config(std::move(config)), _running(false)
{
}

RedisWorker::~RedisWorker() = default;

bool RedisWorker::Start()
{
    sw::redis::ConnectionOptions options;
    options.host            = _config.host;
    options.port            = static_cast<int>(_config.port);
    options.password        = _config.password;
    options.db              = static_cast<long long>(_config.db);
    options.connect_timeout = REDIS_TIMEOUT;
    options.socket_timeout  = REDIS_TIMEOUT;

    try {
        _redis = std::make_unique<sw::redis::Redis>(options);
        _redis->ping();
        LogInfo("RedisWorker: connected to ", _config.host, ":", _config.port, " (db ", _config.db, ")");
    }
    catch (const sw::redis::Error& e) {
        LogError("RedisWorker: Redis connect failed (", _config.host, ":", _config.port, "): ", e.what());
        _redis.reset();
        return false;
    }

    _running = true;
    _thread = std::thread(&RedisWorker::Run, this);
    return true;
}

void RedisWorker::Stop()
{
    _running = false;
    _job_queue.NotifyStop();
    if (_thread.joinable()) _thread.join();
    _redis.reset();
}

void RedisWorker::Run()
{
    RedisJob job;
    while (_job_queue.WaitAndPop(job, _running)) {
        Execute(job);
    }
}

// 작업 1건 실행. Redis 오류는 여기서 잡아 실패로 처리(워커 스레드가 죽지 않음)
void RedisWorker::Execute(RedisJob& job)
{
    // 장애 중: 다시 시도할 시각 전이면 보내지 않고 바로 실패(장애 동안 작업 1건/초만 시간 초과를 기다림)
    if (_consecutive_failures > 0 && std::chrono::steady_clock::now() < _retry_after) {
        ++_consecutive_failures;
        if (job.on_failure) job.on_failure();
        return;
    }
    try {
        job.work(*_redis);
    }
    catch (const std::exception& e) {   // sw::redis::Error 포함
        if (_consecutive_failures++ == 0) {
            LogError("RedisWorker: command failed: ", e.what(), " (logging suppressed until recovery)");
        }
        _retry_after = std::chrono::steady_clock::now() + RETRY_INTERVAL;
        if (job.on_failure) job.on_failure();
        return;
    }
    if (_consecutive_failures > 0) {
        LogInfo("RedisWorker: recovered after ", _consecutive_failures, " failed commands");
        _consecutive_failures = 0;
    }
}
