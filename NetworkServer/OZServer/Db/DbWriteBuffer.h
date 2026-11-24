#pragma once
#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <unordered_map>
#include <utility>
#include "Db/DbJobQueue.h"
#include "Common/Log.h"

// 지연 쓰기 종류. 추가 = 여기 한 줄 + Data/DbWriteKinds.cpp 표에 한 줄(주기, 개수, 묶음 쓰기 함수)
enum class WriteKind : uint8_t {
    LoginDate,   // user.login_date 갱신(키 = user.id, 값 사용 안 함 — 내보낼 때 NOW())
    Count
};
inline constexpr size_t WRITE_KIND_COUNT = static_cast<size_t>(WriteKind::Count);

// 키(대상 id) -> 값. 같은 키는 마지막 값만 남는다
using DbWriteEntries = std::unordered_map<int64_t, int64_t>;

struct DbWriteKindSpec {
    const char*               name;
    std::chrono::milliseconds interval;    // 첫 항목이 들어온 뒤 이 시간이 지나면 내보냄
    size_t                    max_items;   // 이만큼 모이면 바로 내보냄(묶음 1건의 최대 크기)
    // DB 워커 스레드에서 묶음 1건 실행. 실패는 예외(mysqlx::Error) — 버퍼가 잡아서 로그
    void (*write)(mysqlx::Session& session, const DbWriteEntries& entries);
};
using DbWriteKindSpecs = std::array<DbWriteKindSpec, WRITE_KIND_COUNT>;

struct DbWriteStats {
    uint64_t batches = 0;   // 내보낸 묶음(DB 작업) 수
    uint64_t rows = 0;      // 묶음에 담긴 항목 수
};

// 지연 쓰기 큐: 자주 반복되고 바로 반영하지 않아도 되는 DB 쓰기를 모아서 묶음 1건으로 처리(GameLoop 스레드 전용, 락 없음).
// - 모으기: 종류별 키 -> 값(같은 키는 덮어씀). 버퍼 크기 <= 키 수.
// - 내보내기: 주기(interval) 또는 개수(max_items) 중 먼저 오는 쪽. 버퍼를 move해서 DbJob 1건으로(복사 없음).
// - 즉시 쓰기: Flush/FlushAll(모인 것을 지금), WriteNow(모인 것 먼저 + 이어서 그 작업).
// - 순서: 전용 큐를 워커 1개가 처리(main.cpp) -> 넣은 순서대로 실행되어 예전 값이 새 값을 덮어쓰지 않음.
// 한계: 비정상 종료 시 아직 내보내지 않은 최대 1주기분은 유실 — 유실되면 안 되는 데이터(재화·아이템 등)는 넣지 않거나 WriteNow.
// 묶음 실행 실패는 LogError 후 버림(재시도 없음, MySQL 재연결 없음 정책과 같음).
class DbWriteBuffer {
public:
    using Clock = std::chrono::steady_clock;

    DbWriteBuffer(DbJobQueue& queue, const DbWriteKindSpecs& specs) : _queue(queue), _specs(specs) {}

    void Put(WriteKind kind, int64_t key, int64_t value = 0) {
        const size_t index = static_cast<size_t>(kind);
        Buffer& buffer = _buffers[index];
        if (buffer.entries.empty()) buffer.deadline = Clock::now() + _specs[index].interval;
        buffer.entries.insert_or_assign(key, value);
        if (buffer.entries.size() >= _specs[index].max_items) Flush(kind);
    }

    // 매 틱: 주기가 지난 종류를 내보냄
    void Update(Clock::time_point now) {
        for (size_t i = 0; i < WRITE_KIND_COUNT; ++i) {
            if (!_buffers[i].entries.empty() && now >= _buffers[i].deadline) Flush(static_cast<WriteKind>(i));
        }
    }

    void Flush(WriteKind kind) {
        const size_t index = static_cast<size_t>(kind);
        Buffer& buffer = _buffers[index];
        if (buffer.entries.empty()) return;

        const DbWriteKindSpec& spec = _specs[index];
        ++_stats.batches;
        _stats.rows += buffer.entries.size();
        _queue.Push({ [write = spec.write, name = spec.name, entries = std::exchange(buffer.entries, {})](mysqlx::Session& session) {
            try {
                write(session, entries);
            }
            catch (const std::exception& e) {   // mysqlx::Error 포함
                LogError("DbWriteBuffer: ", name, " batch failed (", entries.size(), " rows dropped): ", e.what());
            }
        } });
    }

    // 종료 시(GameLoop 정지 후, DB 워커 정지 전) 호출 -> 정상 종료면 유실 없음
    void FlushAll() {
        for (size_t i = 0; i < WRITE_KIND_COUNT; ++i) Flush(static_cast<WriteKind>(i));
    }

    // 지금 바로 써야 하는 같은 종류의 쓰기: 모인 것을 먼저 내보낸 뒤 이어서 넣음(예전 값이 나중에 덮어쓰는 역전 없음)
    void WriteNow(WriteKind kind, DbJob job) {
        Flush(kind);
        _queue.Push(std::move(job));
    }

    size_t GetPendingCount(WriteKind kind) const { return _buffers[static_cast<size_t>(kind)].entries.size(); }
    // 구간 집계를 가져가며 0으로 초기화([Stats] 출력용)
    DbWriteStats TakeStats() { return std::exchange(_stats, {}); }

private:
    struct Buffer {
        DbWriteEntries    entries;
        Clock::time_point deadline{};
    };

    DbJobQueue&                            _queue;
    const DbWriteKindSpecs                 _specs;
    std::array<Buffer, WRITE_KIND_COUNT>   _buffers;
    DbWriteStats                           _stats;
};
