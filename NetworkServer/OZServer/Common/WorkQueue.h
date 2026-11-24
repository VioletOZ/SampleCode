#pragma once
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <queue>

// 한 스레드(GameLoop) -> 워커 스레드(들)로 작업을 넘기는 블로킹 큐. DB 워커 풀과 Redis 워커가 같이 쓴다.
// 워커는 할 일이 없으면 블로킹 대기(바쁜 대기 없음).
template<typename T>
class WorkQueue {
public:
    void Push(T&& item) {
        std::lock_guard<std::mutex> lock(_mutex);
        _queue.push(std::move(item));
        _cv.notify_one();
    }

    // 작업이 생길 때까지 블로킹. running이 false면 남은 작업을 모두 내준 뒤 false(종료 처리용)
    bool WaitAndPop(T& out, const std::atomic<bool>& running) {
        std::unique_lock<std::mutex> lock(_mutex);
        _cv.wait(lock, [&] { return !_queue.empty() || !running; });
        if (_queue.empty()) return false;
        out = std::move(_queue.front());
        _queue.pop();
        return true;
    }

    // running=false로 바꾼 뒤 호출. 워커가 "조건 확인 ~ 대기 진입" 사이에 있으면 알림이 유실될 수 있으므로
    // 같은 뮤텍스를 잠갔다 풀어서 그 구간이 끝난 뒤에 알림을 보냄(락은 즉시 해제 — 종료 시 1회만 호출되어 병목 없음).
    // notify는 락 밖에서 호출해 깨어난 워커가 다시 락을 기다리지 않도록 함.
    void NotifyStop() {
        { std::lock_guard<std::mutex> lock(_mutex); }
        _cv.notify_all();
    }

private:
    std::queue<T> _queue;
    std::mutex _mutex;
    std::condition_variable _cv;
};
