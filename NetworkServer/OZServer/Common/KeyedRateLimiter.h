#pragma once
#include <string>
#include <unordered_map>
#include "Common/TokenBucket.h"

// 키(IP, 로그인 ID 등)별 토큰 버킷 묶음. 처음 쓰는 키는 가득 찬 버킷으로 시작한다.
// 가득 찬 버킷은 기본 상태와 같으므로 Prune으로 지워 메모리가 계속 늘지 않게 한다.
// 스레드 안전하지 않음 — GameLoop 스레드 전용.
class KeyedRateLimiter {
public:
    using Clock = TokenBucket::Clock;

    explicit KeyedRateLimiter(RateLimit limit) : _limit(limit) {}

    // 키의 토큰 1개를 꺼낸다. 비어 있으면 false
    bool TryTake(const std::string& key, Clock::time_point now)
    {
        if (!_limit.Enabled()) return true;
        return _buckets.try_emplace(key, _limit, now).first->second.TryTake(now);
    }

    // 꺼내지 않고 남았는지만 확인(없는 키는 가득 찬 것으로 봄, 항목을 만들지 않음)
    bool HasToken(const std::string& key, Clock::time_point now) const
    {
        const auto it = _buckets.find(key);
        return it == _buckets.end() || it->second.HasToken(now);
    }

    // 다시 가득 찬(최근에 쓰지 않은) 키 제거
    void Prune(Clock::time_point now)
    {
        std::erase_if(_buckets, [now](const auto& entry) { return entry.second.IsFull(now); });
    }

    size_t Size() const { return _buckets.size(); }

private:
    RateLimit _limit;
    std::unordered_map<std::string, TokenBucket> _buckets;
};
