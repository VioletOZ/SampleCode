#pragma once
#include <cstddef>
#include <cstdint>
#include <deque>
#include <utility>

// 로그인 대기열(FIFO + 상한). 세션/DB와 무관한 순수 자료구조라 단위 테스트 가능.
// 사용 측(UserManager)은 GameLoop 스레드에서만 접근 -> 락 없음.
template <typename Entry>
class LoginQueue {
public:
    explicit LoginQueue(size_t capacity) : _capacity(capacity) {}

    // 상한에 도달했으면 false(항목은 이동되지 않음)
    bool TryPush(Entry&& entry) {
        if (_entries.size() >= _capacity) return false;
        _entries.push_back(std::move(entry));
        return true;
    }

    bool TryPop(Entry& out) {
        if (_entries.empty()) return false;
        out = std::move(_entries.front());
        _entries.pop_front();
        return true;
    }

    size_t Size() const { return _entries.size(); }
    bool Empty() const { return _entries.empty(); }

    // is_alive(entry)가 false인 항목은 on_removed(entry) 후 제거(순서 유지),
    // 남은 항목에는 앞에서부터 on_position(entry, 순번(1부터), 전체 인원)을 호출
    template <typename IsAlive, typename OnRemoved, typename OnPosition>
    void PruneAndNumber(IsAlive is_alive, OnRemoved on_removed, OnPosition on_position) {
        size_t kept = 0;
        for (size_t i = 0; i < _entries.size(); ++i) {
            if (!is_alive(_entries[i])) {
                on_removed(_entries[i]);
                continue;
            }
            if (kept != i) _entries[kept] = std::move(_entries[i]);
            ++kept;
        }
        _entries.resize(kept);

        const uint32_t total = static_cast<uint32_t>(kept);
        for (size_t i = 0; i < kept; ++i) {
            on_position(_entries[i], static_cast<uint32_t>(i + 1), total);
        }
    }

private:
    const size_t _capacity;
    std::deque<Entry> _entries;
};
