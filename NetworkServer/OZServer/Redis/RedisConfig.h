#pragma once
#include <string>

// Redis 접속 설정(server.conf redis.*). 기본값은 WSL2/로컬에 설치한 redis-server(인증 없음, 0번 DB)
struct RedisConfig {
    std::string  host = "127.0.0.1";
    std::string  password;            // 빈 값 = 인증 안 함
    unsigned int port = 6379;
    unsigned int db   = 0;            // 0~15(redis-server 기본 DB 수 16)
};
