#include "stdafx.h"
#include "Db/DbWorker.h"
#include <algorithm>
#include <mysqlx/xdevapi.h>
#include "Common/Log.h"

DbWorker::DbWorker(DbJobQueue& job_queue, DbResultQueue& result_queue, DbConfig config)
    : _job_queue(job_queue), _result_queue(result_queue), _config(std::move(config)), _running(false)
{
}

DbWorker::~DbWorker() = default;

bool DbWorker::Start()
{
    const unsigned int count = std::max(1u, _config.worker_count);
    _sessions.reserve(count);
    try {
        for (unsigned int i = 0; i < count; ++i) {
            _sessions.push_back(std::make_unique<mysqlx::Session>(
                mysqlx::SessionOption::HOST, _config.host,
                mysqlx::SessionOption::PORT, _config.port,
                mysqlx::SessionOption::USER, _config.user,
                mysqlx::SessionOption::PWD, _config.password,
                mysqlx::SessionOption::DB, _config.database));
        }
    }
    catch (const mysqlx::Error& e) {
        LogError("DbWorker: MySQL connect failed (", _config.host, ":", _config.port, ", db=", _config.database, "): ", e.what());
        _sessions.clear();
        return false;
    }

    _running = true;
    _threads.reserve(count);
    for (auto& session : _sessions) {
        _threads.emplace_back(&DbWorker::Run, this, std::ref(*session));
    }
    return true;
}

// 정지 요청 후에도 큐에 남은 작업은 모두 처리하고 종료(WaitAndPop이 큐가 빌 때까지 작업을 반환)
void DbWorker::Stop()
{
    _running = false;
    _job_queue.NotifyStop();
    for (auto& t : _threads) {
        if (t.joinable()) t.join();
    }
    _threads.clear();
    _sessions.clear();
}

// 실패를 요청 측에 결과로 돌려주는 것은 각 작업의 책임(작업 안에서 잡아 실패 결과 전달).
// 여기 가드는 작업이 놓친 예외로 워커 스레드가 끝나 서버 전체가 종료(std::terminate)되는 것만 막는 최후 방어
void DbWorker::Run(mysqlx::Session& session)
{
    DbJob job;
    while (_job_queue.WaitAndPop(job, _running)) {
        try {
            job.work(session);
        }
        catch (const std::exception& e) {
            LogError("DbWorker: unhandled exception in DB job: ", e.what());
        }
        catch (...) {
            LogError("DbWorker: unhandled unknown exception in DB job");
        }
    }
}
