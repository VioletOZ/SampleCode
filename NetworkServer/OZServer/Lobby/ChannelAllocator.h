#pragma once
#include <algorithm>
#include <cstdint>
#include <vector>

// 로비 채널 배정(채널별 소속 인원 수만 관리). 채널 번호는 1부터.
// 배정: 번호가 가장 낮고 자리가 있는 채널(앞 채널부터 채움), 모두 차 있으면 새 채널.
// 반환: 맨 뒤 채널들이 비면 삭제(중간의 빈 채널은 다음 배정에서 먼저 채워짐).
// 스레드 안전하지 않음 — GameLoop 스레드 전용.
enum class ChannelAssignResult { Ok, NotFound, Full };

class ChannelAllocator {
public:
    explicit ChannelAllocator(uint32_t capacity) : _capacity(std::max<uint32_t>(1, capacity)) {}

    uint32_t Assign()
    {
        for (size_t i = 0; i < _counts.size(); ++i) {
            if (_counts[i] < _capacity) {
                ++_counts[i];
                return static_cast<uint32_t>(i + 1);
            }
        }
        _counts.push_back(1);
        return static_cast<uint32_t>(_counts.size());
    }

    // 지정한 채널에 배정(채널 이동). 있는 채널(1~채널 수)이고 정원 미만일 때만 인원 +1. 새 채널은 만들지 않음
    ChannelAssignResult TryAssign(uint32_t channel)
    {
        if (channel == 0 || channel > _counts.size()) return ChannelAssignResult::NotFound;
        if (_counts[channel - 1] >= _capacity) return ChannelAssignResult::Full;
        ++_counts[channel - 1];
        return ChannelAssignResult::Ok;
    }

    // Assign/TryAssign으로 받은 채널을 1회 반환. 범위 밖/이미 0이면 무시
    void Release(uint32_t channel)
    {
        if (channel == 0 || channel > _counts.size() || _counts[channel - 1] == 0) return;
        --_counts[channel - 1];
        while (!_counts.empty() && _counts.back() == 0) _counts.pop_back();
    }

    size_t   GetChannelCount() const { return _counts.size(); }
    uint32_t GetMemberCount(uint32_t channel) const
    {
        return (channel == 0 || channel > _counts.size()) ? 0 : _counts[channel - 1];
    }

private:
    const uint32_t        _capacity;
    std::vector<uint32_t> _counts;   // [채널 - 1] = 소속 인원(로비 + 방 안)
};
