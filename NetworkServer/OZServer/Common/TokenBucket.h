#pragma once
#include <algorithm>
#include <chrono>

// 레이트리밋 수치: 순간 최대 burst개, 이후 초당 per_sec개씩 회복. burst가 0이면 제한 없음
struct RateLimit {
    double burst;
    double per_sec;

    constexpr bool Enabled() const { return burst > 0; }
};

// 토큰 버킷: 요청 1건 = 토큰 1개. 비어 있으면 거부, 시간이 지나면 per_sec 속도로 burst까지 다시 찬다.
// 스레드 안전하지 않음 — 소유 스레드(세션 strand 또는 GameLoop) 하나에서만 사용. 시간은 인자로 받는다(테스트 가능).
class TokenBucket {
public:
    using Clock = std::chrono::steady_clock;

    explicit TokenBucket(RateLimit limit, Clock::time_point now = Clock::now())
        : _limit(limit), _tokens(limit.burst), _last(now) {}

    // 토큰 amount개(기본 1)를 꺼낸다. 모자라면 꺼내지 않고 false(제한 없음이면 항상 true).
    // amount는 요청 수 외의 단위로도 쓸 수 있다(예: 방 게임 이동 거리 — Room/RoomGame.h)
    bool TryTake(Clock::time_point now, double amount = 1.0)
    {
        if (!_limit.Enabled()) return true;
        _tokens = TokensAt(now);
        _last = std::max(_last, now);
        if (_tokens < amount) return false;
        _tokens -= amount;
        return true;
    }

    // 꺼내지 않고 남아 있는지만 확인
    bool HasToken(Clock::time_point now) const { return !_limit.Enabled() || TokensAt(now) >= 1.0; }

    // 가득 참 = 최근에 쓰지 않음 -> 키별 제한에서 정리 대상
    bool IsFull(Clock::time_point now) const { return TokensAt(now) >= _limit.burst; }

private:
    // 시간이 되돌아간 경우(now < _last)에는 채우지 않음
    double TokensAt(Clock::time_point now) const
    {
        if (now <= _last) return _tokens;
        const double elapsed = std::chrono::duration<double>(now - _last).count();
        return std::min(_limit.burst, _tokens + elapsed * _limit.per_sec);
    }

    RateLimit         _limit;
    double            _tokens;
    Clock::time_point _last;
};
