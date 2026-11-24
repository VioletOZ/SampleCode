#pragma once
#include <queue>
#include <mutex>
#include "Db/DbJob.h"

// DB 워커 스레드 -> GameLoop 스레드로 완료 결과를 넘기는 큐.
// GameLoop이 매 틱마다 PopAll로 꺼내서 콜백을 자기 스레드에서 실행함
// (Message/MessageQueue.h와 동일한 스왑 기반 PopAll 패턴).
class DbResultQueue {
public:
    void Push(DbResult&& result) {
        std::lock_guard<std::mutex> lock(_mutex);
        _queue.push(std::move(result));
    }

    std::queue<DbResult> PopAll() {
        std::queue<DbResult> tmp;
        std::lock_guard<std::mutex> lock(_mutex);
        std::swap(tmp, _queue);
        return tmp;
    }

private:
    std::queue<DbResult> _queue;
    std::mutex _mutex;
};
