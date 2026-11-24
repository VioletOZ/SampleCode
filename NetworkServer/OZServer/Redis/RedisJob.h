#pragma once
#include <functional>

namespace sw::redis {
class Redis;
}

// Redis 워커 스레드에서 실행될 작업 단위. redis는 워커가 소유한 전용 커넥션.
// Redis 오류(연결 끊김, 시간 초과 등)로 work가 예외를 던지면 워커가 on_failure를 대신 호출한다
// (조회 작업은 여기서 "없음" 결과를 GameLoop로 돌려 기다리는 쪽이 멈추지 않게 함. 쓰기 작업은 비워 둠).
struct RedisJob {
    std::function<void(sw::redis::Redis& redis)> work;
    std::function<void()>                       on_failure;
};
