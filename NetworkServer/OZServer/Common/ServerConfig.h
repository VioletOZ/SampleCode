#pragma once
#include <algorithm>
#include <charconv>
#include <cstdint>
#include <functional>
#include <istream>
#include <set>
#include <string>
#include <string_view>
#include "Common/Log.h"
#include "Db/DbConfig.h"
#include "Redis/RedisConfig.h"

// 서버 실행 설정(server.conf). 형식: 한 줄에 "key = value", '#'으로 시작하는 줄/빈 줄 무시.
// 줄 중간 주석은 없음(비밀번호에 '#'이 들어갈 수 있어 값은 그대로 사용). 생략한 키는 아래 기본값.
struct ServerConfig {
    uint16_t port       = 12345;
    unsigned io_threads = 4;
    LogLevel log_level  = LogLevel::Info;
    unsigned log_retention_days = DEFAULT_LOG_RETENTION_DAYS;   // 날짜별 로그 파일 보관 일수(0 = 삭제 안 함)
    unsigned user_cache_ttl_seconds = 86400;   // 유저 정보 Redis 캐시 보관 시간(로그인마다 다시 연장, 기본 1일)
    DbConfig    db;
    RedisConfig redis;
};

// 숫자 키와 허용 범위
struct ConfigNumberKey {
    std::string_view name;
    unsigned min;
    unsigned max;
};
inline constexpr ConfigNumberKey CONFIG_NUMBER_KEYS[] = {
    { "port", 1, 65535 },        { "io_threads", 1, 256 },  { "log_retention_days", 0, 3650 },
    { "db.port", 1, 65535 },     { "db.workers", 1, 256 },
    { "redis.port", 1, 65535 },  { "redis.db", 0, 15 },     { "user_cache_ttl_seconds", 60, 2592000 },
};

// 앞뒤 공백/탭/CR 제거(CRLF 파일 대응)
inline std::string_view TrimConfigText(std::string_view text)
{
    constexpr std::string_view SPACES = " \t\r";
    const size_t begin = text.find_first_not_of(SPACES);
    if (begin == std::string_view::npos) return {};
    return text.substr(begin, text.find_last_not_of(SPACES) - begin + 1);
}

// 키 하나를 config에 반영. 모르는 키 / 빈 값(비밀번호 제외) / 범위 밖 숫자 / 잘못된 레벨이면 error에 이유를 넣고 false
inline bool ApplyServerConfigValue(ServerConfig& config, std::string_view key, std::string_view value, std::string& error)
{
    std::string* text = key == "db.host"     ? &config.db.host
                      : key == "db.user"     ? &config.db.user
                      : key == "db.password" ? &config.db.password
                      : key == "db.database" ? &config.db.database
                      : key == "redis.host"  ? &config.redis.host
                      : key == "redis.password" ? &config.redis.password : nullptr;
    if (text) {
        if (value.empty() && !key.ends_with(".password")) {
            error = std::string(key) + " must not be empty";
            return false;
        }
        text->assign(value);
        return true;
    }
    if (key == "log_level") {
        if (ParseLogLevel(value, config.log_level)) return true;
        error = "log_level must be debug|info|warn|error";
        return false;
    }

    // 숫자 키(범위는 CONFIG_NUMBER_KEYS)
    const auto* spec = std::find_if(std::begin(CONFIG_NUMBER_KEYS), std::end(CONFIG_NUMBER_KEYS),
                                    [key](const ConfigNumberKey& k) { return k.name == key; });
    if (spec == std::end(CONFIG_NUMBER_KEYS)) {
        error = "unknown key '" + std::string(key) + "'";
        return false;
    }
    unsigned number = 0;
    const char* end = value.data() + value.size();
    const auto [parsed_end, ec] = std::from_chars(value.data(), end, number);
    if (ec != std::errc{} || parsed_end != end || number < spec->min || number > spec->max) {
        error = std::string(key) + " must be a number in " + std::to_string(spec->min) + ".." + std::to_string(spec->max);
        return false;
    }
    if (key == "port")                    config.port = static_cast<uint16_t>(number);
    else if (key == "io_threads")         config.io_threads = number;
    else if (key == "log_retention_days") config.log_retention_days = number;
    else if (key == "db.port")            config.db.port = number;
    else if (key == "db.workers")         config.db.worker_count = number;
    else if (key == "redis.port")         config.redis.port = number;
    else if (key == "redis.db")           config.redis.db = number;
    else                                  config.user_cache_ttl_seconds = number;
    return true;
}

// 설정 전체를 읽어 config에 반영. 첫 오류에서 멈추고 "line N: 이유"를 error에 넣어 false(config는 일부만 바뀌었을 수 있음)
inline bool ParseServerConfig(std::istream& in, ServerConfig& config, std::string& error)
{
    std::set<std::string, std::less<>> seen_keys;
    std::string line;
    for (int line_no = 1; std::getline(in, line); ++line_no) {
        std::string_view text(line);
        if (line_no == 1 && text.starts_with("\xEF\xBB\xBF")) text.remove_prefix(3);   // 메모장 등이 붙이는 UTF-8 BOM
        text = TrimConfigText(text);
        if (text.empty() || text.front() == '#') continue;

        const size_t eq = text.find('=');
        const std::string_view key = TrimConfigText(text.substr(0, eq));
        if (eq == std::string_view::npos || key.empty()) {
            error = "line " + std::to_string(line_no) + ": expected 'key = value'";
            return false;
        }
        if (!seen_keys.emplace(key).second) {
            error = "line " + std::to_string(line_no) + ": duplicate key '" + std::string(key) + "'";
            return false;
        }
        if (!ApplyServerConfigValue(config, key, TrimConfigText(text.substr(eq + 1)), error)) {
            error = "line " + std::to_string(line_no) + ": " + error;
            return false;
        }
    }
    return true;
}
