#pragma once
#include <chrono>
#include <cstdint>
#include <algorithm>

// GameLoop 틱 처리 지표(부하 테스트/운영 관찰용). GameLoop 스레드에서만 사용 -> 락 불필요.
// Record()로 틱마다 누적하고, TakeSummary()로 구간 요약을 꺼내면서 초기화한다.
struct TickSummary {
    uint32_t ticks = 0;          // 구간 내 틱 수
    double   avg_ms = 0.0;       // 틱 처리 시간 평균
    double   max_ms = 0.0;       // 틱 처리 시간 최대
    uint32_t overruns = 0;       // 처리 시간이 틱 예산(TICK_DURATION) 이상인 틱 수
    uint64_t messages = 0;       // 구간 내 처리한 메시지(패킷 + 연결 종료 이벤트) 수
    uint32_t max_messages = 0;   // 한 틱에서 처리한 최대 메시지 수
    uint64_t skipped = 0;        // 너무 밀려서 실행하지 않고 건너뛴 틱 수(NextTickTime)
};

// 다음 틱 예정 시각 계산 결과
struct TickSchedule {
    std::chrono::steady_clock::time_point next;
    uint32_t skipped = 0;
};

// scheduled: 방금 실행한 틱의 예정 시각, now: 틱 처리를 마친 현재 시각.
// 밀린 시간이 max_lag 이하이면 예정대로 다음 틱(짧은 지연은 따라잡아 평균 TPS 유지),
// 넘으면 밀린 틱을 몰아서 실행하지 않고 지금 바로 다음 틱을 돌도록 재설정하고 건너뛴 틱 수를 돌려준다.
inline TickSchedule NextTickTime(std::chrono::steady_clock::time_point scheduled,
                                 std::chrono::steady_clock::time_point now,
                                 std::chrono::steady_clock::duration tick,
                                 std::chrono::steady_clock::duration max_lag)
{
    const auto next = scheduled + tick;
    const auto lag = now - next;
    if (lag <= max_lag) return { next, 0 };
    return { now, static_cast<uint32_t>(lag / tick) };
}

class TickStats {
public:
    explicit TickStats(std::chrono::microseconds budget) : _budget(budget) {}

    void Record(std::chrono::microseconds elapsed, uint32_t messages) {
        ++_ticks;
        _total += elapsed;
        _max = std::max(_max, elapsed);
        if (elapsed >= _budget) ++_overruns;
        _messages += messages;
        _max_messages = std::max(_max_messages, messages);
    }

    void RecordSkipped(uint32_t ticks) { _skipped += ticks; }

    TickSummary TakeSummary() {
        TickSummary summary;
        summary.ticks = _ticks;
        summary.avg_ms = _ticks ? ToMs(_total) / _ticks : 0.0;
        summary.max_ms = ToMs(_max);
        summary.overruns = _overruns;
        summary.messages = _messages;
        summary.max_messages = _max_messages;
        summary.skipped = _skipped;
        *this = TickStats(_budget);
        return summary;
    }

private:
    static double ToMs(std::chrono::microseconds us) { return us.count() / 1000.0; }

    std::chrono::microseconds _budget;
    std::chrono::microseconds _total{ 0 };
    std::chrono::microseconds _max{ 0 };
    uint32_t _ticks = 0;
    uint32_t _overruns = 0;
    uint64_t _messages = 0;
    uint32_t _max_messages = 0;
    uint64_t _skipped = 0;
};
