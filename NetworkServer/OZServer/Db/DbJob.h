#pragma once
#include <functional>
#include <mysqlx/common/api.h>

// 실제 라이브러리 헤더(mysqlx/xdevapi.h 등)와 동일한 ABI 매크로로 전방 선언해야
// mysqlx::Session이 inline namespace(mysqlx::abi2::r0) 너머의 진짜 타입과 같은
// 엔티티를 가리킴. 직접 "namespace mysqlx { class Session; }"로 선언하면
// 서로 다른 타입으로 취급되어 모호성 에러가 발생함.
namespace mysqlx {
MYSQLX_ABI_BEGIN(2,0)
class Session;
MYSQLX_ABI_END(2,0)
}

// DB 워커 스레드에서 실행될 작업 단위. session은 워커가 소유한 전용 커넥션(X DevAPI).
struct DbJob {
    std::function<void(mysqlx::Session& session)> work;
};

// GameLoop 스레드에서 실행될 완료 콜백 단위.
struct DbResult {
    std::function<void()> callback;
};
