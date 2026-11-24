#pragma once
#include <chrono>

// 단계적 종료: 종료 신호 -> 새 접속 차단 + 모든 요청 거부(ERR_SERVER_SHUTTING_DOWN) -> 이미 접수된 작업 처리 -> 세션 닫고 종료.
// 세션을 닫는 시점: 종료 시작 후 SHUTDOWN_GRACE가 지났고 처리 중인 작업이 없을 때.
// 작업이 끝나지 않아도(DB 멈춤 등) SHUTDOWN_MAX_WAIT가 지나면 종료.
inline constexpr auto SHUTDOWN_GRACE    = std::chrono::seconds(10);
inline constexpr auto SHUTDOWN_MAX_WAIT = std::chrono::seconds(60);

inline bool ShouldFinishShutdown(std::chrono::steady_clock::duration elapsed, bool has_pending_work)
{
    if (elapsed >= SHUTDOWN_MAX_WAIT) return true;
    return elapsed >= SHUTDOWN_GRACE && !has_pending_work;
}
