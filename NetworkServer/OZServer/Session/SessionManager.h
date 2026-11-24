#pragma once
#include <vector>
#include <mutex>
#include <memory>
#include <unordered_set>
#include "Session.h"

class SessionManager {
public:
    void AddSession(SessionPtr session) {
        std::lock_guard<std::mutex> lock(_mutex);
        _sessions.insert(std::move(session));
    }

    // 해시 집합이라 O(1)(이전 vector는 끊길 때마다 전체 탐색 + 삭제)
    void RemoveSession(const SessionPtr& session) {
        std::lock_guard<std::mutex> lock(_mutex);
        _sessions.erase(session);
    }

    size_t GetSessionCount() const {
        std::lock_guard<std::mutex> lock(_mutex);
        return _sessions.size();
    }

    std::vector<SessionPtr> GetAllSessions() const {
        std::lock_guard<std::mutex> lock(_mutex);
        return { _sessions.begin(), _sessions.end() };  // 복사본 반환(락 밖에서 안전하게 순회). 순서는 정해지지 않음
    }

private:
    std::unordered_set<SessionPtr> _sessions;
    mutable std::mutex _mutex;  // 세션 추가/제거/조회 시 사용(로비 브로드캐스트는 LobbyManager가 GameLoop에서 처리)
};
