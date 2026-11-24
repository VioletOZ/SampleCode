#pragma once
#include <string>

// DB 접속 설정. 값은 server.conf(db.*)에서 읽는다 — 계정/비밀번호는 코드에 두지 않음(기본값 빈 값).
// X DevAPI는 classic 3306이 아니라 X Plugin 포트(기본 33060)로 접속함.
// MySQL 서버에 X Plugin이 활성화돼 있어야 함(8.0+ 기본 포함, 비활성화돼 있으면 서버에서 별도 설정 필요).
struct DbConfig {
    std::string  host     = "127.0.0.1";
    std::string  user;
    std::string  password;
    std::string  database = "account";
    unsigned int port     = 33060;
    unsigned int worker_count = 8;   // DB 워커 스레드 수 = 커넥션 수(스레드마다 전용 커넥션 1개). 측정: 1->8개에서 로그인 처리량 ~115->~550건/초(loadtest.md)
};
