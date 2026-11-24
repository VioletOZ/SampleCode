#pragma once
#include <atomic>
#include <cctype>
#include <charconv>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>

// 비동기 로거: 호출 스레드는 한 줄을 만들어 큐에 넣기만 하고, 전용 로그 스레드 1개가 출력 대상(콘솔/파일)에 쓴다.
// -> 콘솔/파일 쓰기가 GameLoop·I/O·DB 스레드를 멈추지 않고, 여러 스레드의 출력이 섞이지 않는다.
// 꺼진 레벨은 문자열을 만들지 않고 바로 반환(atomic 읽기 1회).

enum class LogLevel : uint8_t { Debug, Info, Warn, Error };

inline const char* LogLevelName(LogLevel level)
{
    switch (level) {
    case LogLevel::Debug: return "DEBUG";
    case LogLevel::Info:  return "INFO ";
    case LogLevel::Warn:  return "WARN ";
    case LogLevel::Error: return "ERROR";
    }
    return "?????";
}

// "debug|info|warn|error"(대소문자 무시). 잘못된 값이면 false(out 변경 없음)
inline bool ParseLogLevel(std::string_view text, LogLevel& out)
{
    std::string lower(text);
    for (char& c : lower) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    if (lower == "debug")      out = LogLevel::Debug;
    else if (lower == "info")  out = LogLevel::Info;
    else if (lower == "warn")  out = LogLevel::Warn;
    else if (lower == "error") out = LogLevel::Error;
    else return false;
    return true;
}

inline bool IsLogEnabled(LogLevel current, LogLevel level)
{
    return level >= current;
}

// "[2026-11-17 12:34:56.789] [INFO ] message"
inline std::string FormatLogLine(LogLevel level, std::chrono::system_clock::time_point time, std::string_view message)
{
    const std::time_t seconds = std::chrono::system_clock::to_time_t(time);
    const auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(time.time_since_epoch()).count() % 1000;
    std::tm tm_local{};
    localtime_s(&tm_local, &seconds);

    char stamp[32];
    const size_t len = std::strftime(stamp, sizeof(stamp), "%Y-%m-%d %H:%M:%S", &tm_local);
    std::snprintf(stamp + len, sizeof(stamp) - len, ".%03d", static_cast<int>(millis));

    std::string line;
    line.reserve(36 + message.size());
    line.append("[").append(stamp).append("] [").append(LogLevelName(level)).append("] ").append(message);
    return line;
}

struct LogEntry {
    LogLevel    level;
    std::string line;   // FormatLogLine 결과(시각은 큐에 넣을 때 기록)
};

class Logger {
public:
    // 출력 대상: 로그 스레드에서 묶음 단위로 호출(시작 전/종료 후에는 호출 스레드에서, 뮤텍스로 직렬화)
    using Sink = std::function<void(const std::vector<LogEntry>&)>;
    static constexpr size_t MAX_PENDING_LOG_LINES = 100000;   // 넘으면 새 줄을 버리고 버린 수를 기록(메모리 보호)

    explicit Logger(Sink sink, size_t max_pending = MAX_PENDING_LOG_LINES)
        : _sink(std::move(sink)), _max_pending(max_pending) {}
    ~Logger() { Stop(); }

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    void SetLevel(LogLevel level) { _level.store(level, std::memory_order_relaxed); }
    // 출력 대상 교체(main에서 설정을 읽은 뒤). 로그 스레드는 락 밖에서 sink를 호출하므로 Start 전에만 바꿀 수 있음(실행 중이면 false)
    bool SetSink(Sink sink) {
        std::lock_guard<std::mutex> lock(_mutex);
        if (_running) return false;
        _sink = std::move(sink);
        return true;
    }
    bool IsEnabled(LogLevel level) const { return IsLogEnabled(_level.load(std::memory_order_relaxed), level); }

    void Start() {
        std::lock_guard<std::mutex> lock(_mutex);
        if (_running) return;
        _running = true;
        _thread = std::thread(&Logger::Run, this);
    }

    // 남은 줄을 모두 쓴 뒤 로그 스레드 종료. 이후 로그는 호출 스레드에서 바로 씀
    void Stop() {
        {
            std::lock_guard<std::mutex> lock(_mutex);
            if (!_running) return;
            _stopping = true;
        }
        _cv.notify_all();
        if (_thread.joinable()) _thread.join();
    }

    void Write(LogLevel level, std::string_view message) {
        LogEntry entry{ level, FormatLogLine(level, std::chrono::system_clock::now(), message) };
        std::unique_lock<std::mutex> lock(_mutex);
        if (!_running) {                       // 시작 전/종료 후: 유실되지 않도록 바로 출력
            _sink(std::vector<LogEntry>{ std::move(entry) });
            return;
        }
        if (_pending.size() >= _max_pending) {
            ++_dropped;
            return;
        }
        _pending.push_back(std::move(entry));
        lock.unlock();
        _cv.notify_one();
    }

private:
    void Run() {
        std::vector<LogEntry> batch;
        std::unique_lock<std::mutex> lock(_mutex);
        while (true) {
            _cv.wait(lock, [this] { return !_pending.empty() || _dropped > 0 || _stopping; });
            if (_pending.empty() && _dropped == 0 && _stopping) {
                _running = false;              // 같은 락 안에서 전환 -> 이후 Write는 직접 출력(유실 없음)
                _stopping = false;
                return;
            }
            batch.swap(_pending);              // 용량을 주고받아 재사용(매번 할당 없음)
            const uint64_t dropped = std::exchange(_dropped, 0);
            lock.unlock();

            if (dropped > 0) {
                batch.push_back({ LogLevel::Warn, FormatLogLine(LogLevel::Warn, std::chrono::system_clock::now(),
                    "log queue full, " + std::to_string(dropped) + " lines dropped") });
            }
            _sink(batch);
            batch.clear();
            lock.lock();
        }
    }

    Sink                  _sink;
    const size_t          _max_pending;
    std::atomic<LogLevel> _level{ LogLevel::Info };

    std::mutex              _mutex;
    std::condition_variable _cv;
    std::vector<LogEntry>   _pending;
    uint64_t                _dropped = 0;
    bool                    _running = false;
    bool                    _stopping = false;
    std::thread             _thread;
};

// ---- 날짜별 로그 파일 ----
constexpr size_t   LOG_DATE_LENGTH = 10;              // "YYYY-MM-DD"
constexpr unsigned DEFAULT_LOG_RETENTION_DAYS = 30;   // server.conf log_retention_days 기본값(0 = 삭제 안 함)

// FormatLogLine 결과 "[YYYY-MM-DD hh:mm:ss.mmm] ..."의 날짜 부분(형식이 다르면 빈 값)
inline std::string_view LogDateOf(std::string_view line)
{
    return (line.size() > LOG_DATE_LENGTH && line[0] == '[') ? line.substr(1, LOG_DATE_LENGTH) : std::string_view{};
}

// "YYYY-MM-DD"에서 days일 전 날짜("YYYY-MM-DD"). 형식이 다르거나 없는 날짜면 빈 값
inline std::string LogDateMinusDays(std::string_view date, unsigned days)
{
    int year = 0;
    unsigned month = 0, day = 0;
    if (date.size() != LOG_DATE_LENGTH || date[4] != '-' || date[7] != '-') return {};
    const char* text = date.data();
    if (std::from_chars(text, text + 4, year).ptr != text + 4 ||
        std::from_chars(text + 5, text + 7, month).ptr != text + 7 ||
        std::from_chars(text + 8, text + 10, day).ptr != text + 10) return {};
    const std::chrono::year_month_day ymd{ std::chrono::year{ year } / month / day };
    if (!ymd.ok()) return {};
    const std::chrono::year_month_day cutoff{ std::chrono::sys_days{ ymd } - std::chrono::days{ days } };
    char out[16];
    std::snprintf(out, sizeof(out), "%04d-%02u-%02u", static_cast<int>(cutoff.year()),
                  static_cast<unsigned>(cutoff.month()), static_cast<unsigned>(cutoff.day()));
    return out;
}

// 날짜별 로그 파일 이름("server_YYYY-MM-DD.log" / "error_YYYY-MM-DD.log")이고 날짜가 cutoff보다 이전이면 true.
// 다른 형식(기존 server.log 등)은 false
inline bool IsExpiredLogFile(std::string_view file_name, std::string_view cutoff)
{
    for (const std::string_view prefix : { std::string_view("server_"), std::string_view("error_") }) {
        if (file_name.size() != prefix.size() + LOG_DATE_LENGTH + 4 || !file_name.starts_with(prefix) || !file_name.ends_with(".log")) {
            continue;
        }
        const std::string_view date = file_name.substr(prefix.size(), LOG_DATE_LENGTH);
        for (size_t i = 0; i < date.size(); ++i) {
            const bool ok = (i == 4 || i == 7) ? date[i] == '-' : std::isdigit(static_cast<unsigned char>(date[i])) != 0;
            if (!ok) return false;
        }
        return date < cutoff;
    }
    return false;
}

// dir에서 보관 기간이 지난 날짜별 로그 파일 삭제(today 기준 days일보다 이전 날짜). days 0이면 삭제 안 함
inline void RemoveExpiredLogFiles(const std::filesystem::path& dir, std::string_view today, unsigned days)
{
    const std::string cutoff = (days == 0) ? std::string() : LogDateMinusDays(today, days);
    if (cutoff.empty()) return;
    std::error_code ec;
    for (std::filesystem::directory_iterator it(dir, ec), end; !ec && it != end; it.increment(ec)) {
        if (it->is_regular_file(ec) && IsExpiredLogFile(it->path().filename().string(), cutoff)) {
            std::filesystem::remove(it->path(), ec);
        }
    }
}

// 서버 기본 출력: 콘솔 + dir/server_YYYY-MM-DD.log, Warn 이상은 dir/error_YYYY-MM-DD.log에도(실행 디렉터리 기준).
// 날짜는 각 줄의 시각 기준 -> 날짜가 바뀌는 줄(첫 기록 포함)에서 파일을 새로 열고 보관 기간이 지난 파일 삭제.
// error 파일은 Warn 이상이 처음 나올 때 연다(오류 없는 날은 파일 없음)
inline Logger::Sink MakeConsoleFileSink(std::filesystem::path dir = "logs", unsigned retention_days = DEFAULT_LOG_RETENTION_DAYS)
{
    struct Files {
        std::filesystem::path dir;
        unsigned              retention_days = 0;
        std::string           date;     // 지금 열린 파일의 날짜(처음엔 빈 값)
        std::ofstream         server;
        std::ofstream         error;
    };
    auto files = std::make_shared<Files>();
    files->dir = std::move(dir);
    files->retention_days = retention_days;

    return [files](const std::vector<LogEntry>& batch) {
        for (const LogEntry& entry : batch) {
            std::cout << entry.line << '\n';
            const std::string_view date = LogDateOf(entry.line);
            if (!date.empty() && date != files->date) {
                files->date.assign(date);
                files->server.close();
                files->error.close();
                std::error_code ec;
                std::filesystem::create_directories(files->dir, ec);
                files->server.open(files->dir / ("server_" + files->date + ".log"), std::ios::app);
                RemoveExpiredLogFiles(files->dir, files->date, files->retention_days);
            }
            if (files->server) files->server << entry.line << '\n';
            if (entry.level < LogLevel::Warn) continue;
            if (!files->error.is_open() && !files->date.empty()) {
                files->error.open(files->dir / ("error_" + files->date + ".log"), std::ios::app);
            }
            if (files->error) files->error << entry.line << '\n';
        }
        std::cout.flush();
        files->server.flush();
        if (files->error.is_open()) files->error.flush();
    };
}

// 서버 전역 로거(main에서 SetSink(보관 기간 반영)/SetLevel/Start, 종료 시 Stop)
inline Logger& GetLogger()
{
    static Logger logger(MakeConsoleFileSink());
    return logger;
}

// 꺼진 레벨이면 문자열을 만들지 않음. 켜져 있으면 인자를 이어 붙여 한 줄로 기록
template <typename... Args>
void Log(LogLevel level, Args&&... args)
{
    Logger& logger = GetLogger();
    if (!logger.IsEnabled(level)) return;
    std::ostringstream oss;
    (oss << ... << std::forward<Args>(args));
    logger.Write(level, oss.str());
}

template <typename... Args> void LogDebug(Args&&... args) { Log(LogLevel::Debug, std::forward<Args>(args)...); }
template <typename... Args> void LogInfo(Args&&... args)  { Log(LogLevel::Info,  std::forward<Args>(args)...); }
template <typename... Args> void LogWarn(Args&&... args)  { Log(LogLevel::Warn,  std::forward<Args>(args)...); }
template <typename... Args> void LogError(Args&&... args) { Log(LogLevel::Error, std::forward<Args>(args)...); }
