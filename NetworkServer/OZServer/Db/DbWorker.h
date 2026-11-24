#pragma once
#include <thread>
#include <atomic>
#include <string>
#include <memory>
#include <vector>
#include "Db/DbJobQueue.h"
#include "Db/DbResultQueue.h"
#include "Db/DbConfig.h"

// DB 워커 풀: 워커 스레드 N개, 스레드마다 전용 MySQL 커넥션(mysqlx::Session) 1개(커넥션을 스레드 간 공유하지 않음 -> 락 없음).
// 시작 시 커넥션을 미리 만들어 재사용하므로 별도 커넥션 풀이 필요 없음.
// 모든 워커가 같은 DbJobQueue에서 작업을 꺼내 병렬 처리. GameLoop 스레드는 DbJobQueue/DbResultQueue로만 통신함.
// 주의: 작업 간 실행 순서는 보장되지 않음 — 같은 대상(예: 같은 ID)의 작업이 동시에 돌지 않게 하는 것은 요청 측 책임.
class DbWorker {
public:
    DbWorker(DbJobQueue& job_queue, DbResultQueue& result_queue, DbConfig config);
    // unique_ptr<mysqlx::Session>이 여기선 전방 선언 상태라, 소멸자를 .cpp에서 정의해야 함
    // (mysqlx::Session의 완전한 타입이 보이는 곳에서 소멸자 코드가 생성되도록).
    ~DbWorker();

    // 커넥션 N개를 모두 연결한 뒤 스레드 시작. 하나라도 실패하면 false(스레드 시작 안 함)
    bool Start();
    void Stop();

private:
    void Run(mysqlx::Session& session);

    DbJobQueue&    _job_queue;
    DbResultQueue& _result_queue;
    DbConfig       _config;

    std::vector<std::unique_ptr<mysqlx::Session>> _sessions;   // 워커 i는 _sessions[i]만 사용
    std::atomic<bool>                             _running;
    std::vector<std::thread>                      _threads;
};
