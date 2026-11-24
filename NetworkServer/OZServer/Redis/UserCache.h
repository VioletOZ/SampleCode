#pragma once
#include <chrono>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include "Redis/RedisJobQueue.h"
#include "Db/DbResultQueue.h"
#include "Data/UserTable.h"   // UserRow

// 유저 정보 Redis 캐시(cache-aside, 원본은 DB). 키 형식과 Redis 명령은 여기에만 둔다. GameLoop 스레드에서 호출.
// 키 oz:user:{uuid} = HASH { id, nickname }, TTL = ttl(로그인 적중/저장 때마다 다시 연장 -> 안 오는 유저는 자동으로 빠짐).
// 규칙: 유저 정보(id/닉네임)를 바꾸는 기능을 넣으면 DB 수정 후 Invalidate — 캐시는 언제 지워져도 DB에서 다시 채워진다.
class UserCache {
public:
    using GetCallback = std::function<void(std::optional<UserRow> user)>;

    UserCache(RedisJobQueue& job_queue, DbResultQueue& result_queue, std::chrono::seconds ttl)
        : _job_queue(job_queue), _result_queue(result_queue), _ttl(ttl) {}

    // 조회(적중하면 TTL 연장). 다음 틱 이후 GameLoop 스레드에서 callback — 없거나 Redis 오류면 nullopt(호출 측은 DB로)
    void Get(const std::string& uuid, GetCallback callback);
    // 저장(응답 기다리지 않음). Redis 오류면 버림(다음 로그인에서 다시 DB 조회 후 저장)
    void Set(const std::string& uuid, int64_t id, std::string nickname);
    void Invalidate(const std::string& uuid);

private:
    static std::string MakeKey(const std::string& uuid);

    RedisJobQueue&       _job_queue;
    DbResultQueue&       _result_queue;
    std::chrono::seconds _ttl;
};
