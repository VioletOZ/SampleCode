#include "stdafx.h"
#include "Redis/UserCache.h"
#include <array>
#include <charconv>
#include <memory>
#include <utility>
#include <vector>
#include <sw/redis++/redis++.h>
#include "Redis/RedisWorker.h"

namespace {
constexpr const char* FIELD_ID = "id";
constexpr const char* FIELD_NICKNAME = "nickname";
}

std::string UserCache::MakeKey(const std::string& uuid)
{
    return std::string(RedisWorker::KEY_PREFIX) + "user:" + uuid;
}

void UserCache::Get(const std::string& uuid, GetCallback callback)
{
    // work와 on_failure 중 하나만 실행되므로 콜백을 공유(둘 다 GameLoop로 결과 1건을 돌려줌)
    auto shared_callback = std::make_shared<GetCallback>(std::move(callback));
    _job_queue.Push({
        [this, key = MakeKey(uuid), shared_callback](sw::redis::Redis& redis) {
            std::vector<sw::redis::OptionalString> values;
            values.reserve(2);
            redis.hmget(key, { FIELD_ID, FIELD_NICKNAME }, std::back_inserter(values));

            std::optional<UserRow> user;
            if (values.size() == 2 && values[0] && values[1]) {
                UserRow found;
                const std::string& id = *values[0];
                const auto [end, ec] = std::from_chars(id.data(), id.data() + id.size(), found.id);
                if (ec == std::errc{} && end == id.data() + id.size() && found.id > 0) {   // 형식이 틀리면 미적중(DB에서 다시 채움)
                    found.nickname = std::move(*values[1]);
                    user = std::move(found);
                    redis.expire(key, _ttl);
                }
            }
            _result_queue.Push({ [shared_callback, user = std::move(user)]() mutable { (*shared_callback)(std::move(user)); } });
        },
        [this, shared_callback] {
            _result_queue.Push({ [shared_callback] { (*shared_callback)(std::nullopt); } });
        }
    });
}

void UserCache::Set(const std::string& uuid, int64_t id, std::string nickname)
{
    _job_queue.Push({
        [key = MakeKey(uuid), id, nickname = std::move(nickname), ttl = _ttl](sw::redis::Redis& redis) {
            const std::array<std::pair<std::string, std::string>, 2> fields{ {
                { FIELD_ID, std::to_string(id) }, { FIELD_NICKNAME, nickname } } };
            redis.hset(key, fields.begin(), fields.end());
            redis.expire(key, ttl);
        },
        nullptr
    });
}

void UserCache::Invalidate(const std::string& uuid)
{
    _job_queue.Push({ [key = MakeKey(uuid)](sw::redis::Redis& redis) { redis.del(key); }, nullptr });
}
