#pragma once
#include <cstdint>
#include <optional>
#include <string>
#include "Db/DbWriteBuffer.h"   // DbWriteEntries, mysqlx::Session 전방 선언

// user 테이블 1행에서 로그인에 필요한 컬럼(전체 스키마는 NetworkServer/db/schema.sql)
struct UserRow {
    int64_t     id = 0;
    std::string nickname;
};

// user 테이블 쿼리와 행 -> 구조체 매핑을 모은 곳(DAO). 게임 로직은 SQL을 직접 쓰지 않는다.
// DB 워커 스레드의 작업 안에서 잠깐 만들어 쓴다(세션은 워커 소유, 참조만). 실패는 예외(mysqlx::Error) — 호출 측이 처리.
// 쿼리 결과 객체는 각 함수 안에서 해제되어 같은 세션에 읽지 않은 결과가 남지 않는다.
class UserTable {
public:
    explicit UserTable(mysqlx::Session& session) : _session(session) {}

    std::optional<UserRow> FindByUuid(const std::string& uuid);
    bool NicknameExists(const std::string& nickname);
    // login_date = NOW() 포함. id는 자동 증가값
    UserRow Insert(const std::string& uuid, std::string nickname);
    // 지연 쓰기 묶음 1건(키 = user.id): login_date = NOW() — 시각은 내보낼 때 기준(오차 <= 주기)
    void UpdateLoginDates(const DbWriteEntries& entries);

private:
    mysqlx::Session& _session;
};
