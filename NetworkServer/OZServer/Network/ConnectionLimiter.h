#pragma once
#include <atomic>
#include <cstdint>
#include <mutex>
#include <string>
#include <unordered_map>
#include "Network/PacketPolicy.h"

enum class AcquireResult { Ok, ServerFull, IpLimit };

// 레이트리밋 지표(STATS 구간 누적값). I/O 스레드에서 세는 항목
struct ConnectionLimitCounters {
    uint64_t rejected = 0;        // 접속 거부(서버 전체 / IP별 상한)
    uint64_t packet_dropped = 0;  // 세션 패킷 빈도 초과로 버린 패킷
    uint64_t rate_disconnected = 0;  // 패킷 빈도 초과 누적으로 끊은 세션
};

// 동시 접속 수 제한(서버 전체 + IP별). accept(acceptor strand)와 세션 종료(각 세션 strand)가
// 서로 다른 I/O 스레드에서 호출하므로 mutex로 보호한다. 접속/해제 때만 잠그므로 비용이 작다.
// IP별 상한이 0이면 IP별 제한을 쓰지 않고, 루프백 주소는 IP별 제한에서 제외한다(IsIpLimitExempt).
class ConnectionLimiter {
public:
    ConnectionLimiter(size_t max_sessions, size_t max_per_ip)
        : _max_sessions(max_sessions), _max_per_ip(max_per_ip) {}

    AcquireResult TryAcquire(const ClientAddress& address)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        if (_total >= _max_sessions) return AcquireResult::ServerFull;
        if (UsesIpLimit(address)) {
            size_t& count = _per_ip[address.ip];
            if (count >= _max_per_ip) return AcquireResult::IpLimit;
            ++count;
        }
        ++_total;
        return AcquireResult::Ok;
    }

    // TryAcquire가 Ok였던 연결이 끊길 때 정확히 1회 호출
    void Release(const ClientAddress& address)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        if (_total > 0) --_total;
        if (!UsesIpLimit(address)) return;
        const auto it = _per_ip.find(address.ip);
        if (it != _per_ip.end() && --it->second == 0) _per_ip.erase(it);   // 0이 되면 항목 삭제
    }

    size_t GetSessionCount() const { std::lock_guard<std::mutex> lock(_mutex); return _total; }
    size_t GetTrackedIpCount() const { std::lock_guard<std::mutex> lock(_mutex); return _per_ip.size(); }

    void CountRejected()         { _rejected.fetch_add(1, std::memory_order_relaxed); }
    void CountPacketDropped()    { _packet_dropped.fetch_add(1, std::memory_order_relaxed); }
    void CountRateDisconnected() { _rate_disconnected.fetch_add(1, std::memory_order_relaxed); }

    // 지표를 가져가며 0으로 초기화(GameLoop의 [Stats] 출력용)
    ConnectionLimitCounters TakeCounters()
    {
        return { _rejected.exchange(0), _packet_dropped.exchange(0), _rate_disconnected.exchange(0) };
    }

private:
    bool UsesIpLimit(const ClientAddress& address) const { return _max_per_ip > 0 && !IsIpLimitExempt(address); }

    const size_t _max_sessions;
    const size_t _max_per_ip;
    mutable std::mutex _mutex;
    size_t _total = 0;
    std::unordered_map<std::string, size_t> _per_ip;

    std::atomic<uint64_t> _rejected{ 0 };
    std::atomic<uint64_t> _packet_dropped{ 0 };
    std::atomic<uint64_t> _rate_disconnected{ 0 };
};
