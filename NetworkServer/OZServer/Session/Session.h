#pragma once
#include "stdafx.h"

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <algorithm>
#include <array>
#include <atomic>
#include <memory>
#include <utility>

#include "Common/Common.h"
#include "Common/Log.h"
#include "Message/MessageQueue.h"
#include "Network/PacketUtils.h"
#include "Network/PacketPolicy.h"
#include "Network/ConnectionLimiter.h"
#include "Common/TokenBucket.h"
#include "Session/SessionState.h"



class SessionManager;


class Session : public std::enable_shared_from_this<Session>
{
public:
    Session(boost::asio::io_context& io_context, MessageQueue& queue, SessionManager& session_manager, ConnectionLimiter& limiter)
        // 소켓을 세션 strand 위에 생성 -> 읽기/쓰기/닫기 완료 핸들러가 모두 이 strand에서만 실행(I/O 스레드가 여러 개여도 안전)
        : _strand(boost::asio::make_strand(io_context)), _socket(_strand), _check_timer(_strand),
        _current_room_id(0), _queue(queue), _session_manager(session_manager), _limiter(limiter), _header_buf{}, _current_header{0, 0}
    {
    }

    boost::asio::ip::tcp::socket& socket() { return _socket; }

    // accept 직후(Start 전, acceptor strand) 1회 기록 후 불변 -> 어느 스레드에서 읽어도 안전.
    // Start()는 ConnectionLimiter::TryAcquire 성공 후에만 호출되며, 종료(HandleDisconnect) 시 이 주소로 Release
    void SetClientAddress(ClientAddress address) { _client_address = std::move(address); }
    const ClientAddress& GetClientAddress() const { return _client_address; }

    void Start();
    void SendPacket(std::string serialized);
    void SendPacket(std::shared_ptr<const std::string> serialized);

    void DoSendPacket(std::shared_ptr<const std::string> serialized);
    void HandleDisconnect();

    // 문자열은 참조로 반환(복사 없음). 세션 ID는 Start() 이후 불변, 유저 정보는 GameLoop 스레드 전용
    const std::string& GetSessionId() const { return _session_id; }

    void SetCurrentRoomId(uint32_t room_id) { _current_room_id = room_id; }
    uint32_t GetCurrentRoomId() const { return _current_room_id; }

    const std::string& GetUserId() const { return _user_id; }
    const std::string& GetUserName() const { return _user_name; }
    // _user_id/_user_name은 GameLoop 스레드에서만 읽고 씀(I/O 스레드에서 접근 금지)
    void SetUserId(std::string id) { _user_id = std::move(id); }
    void SetUserName(std::string name) { _user_name = std::move(name); }

    bool IsDisconnected() const { return _disconnected.load(); }

    // 다른 스레드(GameLoop)에서 연결 종료를 요청. 소켓 작업은 strand(I/O 스레드)에서만 하도록 넘기고,
    // 보내는 중인 패킷(예: 에러 응답)이 있으면 전송이 끝난 뒤에 끊는다.
    // 기다리는 동안 수신은 멈추고(HandleReadBody), CLOSE_WRITE_TIMEOUT 안에 전송이 끝나지 않으면 강제 종료.
    void Disconnect()
    {
        boost::asio::post(_strand, [self = shared_from_this()] {
            if (self->_disconnected) return;
            if (self->_in_flight.empty() && self->_write_queue.empty()) {
                self->HandleDisconnect();
                return;
            }
            self->_close_after_write = true;
            self->ScheduleCloseDeadline();
        });
    }

    // 다른 스레드(서버 종료 등)에서 즉시 종료 요청. 보내는 중인 패킷을 기다리지 않음
    void Close()
    {
        boost::asio::post(_strand, [self = shared_from_this()] { self->HandleDisconnect(); });
    }

    // 프로토콜 위반 횟수 누적(GameLoop 스레드 전용). 누적 후 값을 반환
    uint32_t AddProtocolViolation() { return ++_protocol_violations; }

    // 로비 채널(GameLoop 스레드 전용, LobbyManager가 관리). 0 = 배정 안 됨(로그인 전)
    uint32_t GetLobbyChannel() const { return _lobby_channel; }
    void SetLobbyChannel(uint32_t channel) { _lobby_channel = channel; }

    // 요청 종류별 빈도 토큰 1개 사용(GameLoop 스레드 전용). 비어 있으면 false, None이면 항상 true
    bool TryTakeRequestToken(RequestLimit limit, std::chrono::steady_clock::time_point now) {
        return limit == RequestLimit::None || _request_buckets[static_cast<size_t>(limit)].TryTake(now);
    }

    // 쓰기는 GameLoop 스레드만, 읽기는 GameLoop + 연결 유지 점검 타이머(I/O 스레드) -> atomic
    SessionState GetState() const { return _state.load(std::memory_order_relaxed); }
    void SetState(SessionState state) { _state.store(state, std::memory_order_relaxed); }

private:
    void ReadHeader()
    {
        boost::asio::async_read(_socket,
            boost::asio::buffer(_header_buf, HEADER_SIZE),
            boost::asio::bind_executor(_strand,
                boost::bind(&Session::HandleReadHeader, shared_from_this(),
                    boost::asio::placeholders::error)));
    }

    void HandleReadHeader(const boost::system::error_code& error)
    {
        if (error) { HandleDisconnect(); return; }
        _current_header = PacketUtils::ParseHeader(_header_buf);
        if (!PacketUtils::IsValidRequestBodySize(_current_header.body_size)) {
            // 악의적/손상된 클라이언트가 거대한 body_size로 메모리 할당을 유발하는 것을 차단(요청 상한 4KB)
            LogWarn("Session ", GetSessionId(), " body_size ", _current_header.body_size,
                    " exceeds MAX_REQUEST_SIZE(", PacketUtils::MAX_REQUEST_SIZE, "), disconnecting");
            HandleDisconnect();
            return;
        }
        _body_buf.assign(_current_header.body_size, '\0');   // 이동된(빈) 버퍼를 이번 크기로 다시 할당
        ReadBody();
    }

    void ReadBody()
    {
        boost::asio::async_read(_socket,
            boost::asio::buffer(_body_buf),
            boost::asio::bind_executor(_strand,
                boost::bind(&Session::HandleReadBody, shared_from_this(),
                    boost::asio::placeholders::error)));
    }

    void HandleReadBody(const boost::system::error_code& error)
    {
        if (error) { HandleDisconnect(); return; }
        if (_close_after_write) return;                  // 종료 대기 중: 큐에 넣지 않고 읽기도 다시 걸지 않음(수신 중단)
        _last_recv = std::chrono::steady_clock::now();   // 패킷을 끝까지 받은 시각(연결 유지 판정)
        if (!_packet_bucket.TryTake(_last_recv)) {       // 빈도 초과: GameLoop로 넘기지 않고 버림(하트비트 포함)
            OnPacketRateExceeded();
            return;
        }
        if (_current_header.type == protobuf::HEARTBEAT) {   // 연결 유지 신호는 GameLoop로 넘기지 않고 여기서 버림
            ReadHeader();
            return;
        }
        // 받은 버퍼를 복사하지 않고 그대로 넘김(이동). 다음 패킷은 ReadBody에서 새 버퍼로 받음
        _queue.Push(shared_from_this(), RawPacket{ _current_header.type, std::move(_body_buf) });
        ReadHeader();
    }

    // 대기열에 쌓인 패킷을 최대 MAX_WRITE_BATCH개까지 묶어 async_write 1회로 전송(scatter-gather, 복사 없음).
    // 전송 중이 아닐 때 들어온 패킷은 1개짜리 묶음으로 바로 나가므로 지연은 늘지 않고, 송신이 밀릴 때만 묶음이 커진다.
    void Write()
    {
        const size_t count = std::min(_write_queue.size(), MAX_WRITE_BATCH);
        for (size_t i = 0; i < count; ++i) {
            _write_buffers.push_back(boost::asio::buffer(*_write_queue.front()));
            _in_flight.push_back(std::move(_write_queue.front()));
            _write_queue.pop_front();
        }

        boost::asio::async_write(_socket, _write_buffers,
            boost::asio::bind_executor(_strand,
                // 쓰기 완료 전까지 세션(과 전송 중인 버퍼)이 해제되지 않도록 shared_ptr로 붙잡음
                [self = shared_from_this()](const boost::system::error_code& error, size_t bytes) {
                    if (error) { self->HandleDisconnect(); return; }
                    self->_pending_bytes -= bytes;
                    self->_in_flight.clear();       // 용량은 유지 -> 다음 묶음에서 재할당 없음
                    self->_write_buffers.clear();
                    if (!self->_write_queue.empty()) self->Write();
                    else if (self->_close_after_write) self->HandleDisconnect();
                }));
    }

    // 연결 유지 점검(strand 전용): CONNECTION_CHECK_INTERVAL마다 무응답/로그인 지연 확인
    void ScheduleConnectionCheck();
    void CheckConnection();
    // 종료 대기(strand 전용): 점검 타이머를 CLOSE_WRITE_TIMEOUT 마감으로 바꿔 걸고, 마감까지 전송이 안 끝나면 종료
    void ScheduleCloseDeadline();
    // 패킷 빈도 초과(strand 전용): 구간 첫 초과만 알리고, 구간 누적 MAX_RATE_DROPS 초과 시 연결 종료
    void OnPacketRateExceeded();

    static constexpr size_t MAX_WRITE_BATCH = 64;

    // REQUEST_RATE_LIMITS 표 순서대로 요청 종류별 버킷 생성(종류를 추가해도 여기는 그대로)
    template<size_t... Index>
    static std::array<TokenBucket, sizeof...(Index)> MakeRequestBuckets(std::index_sequence<Index...>) {
        return { TokenBucket(REQUEST_RATE_LIMITS[Index])... };
    }

    // 선언 순서 중요: _socket이 _strand로 초기화되므로 _strand가 먼저
    boost::asio::strand<boost::asio::io_context::executor_type> _strand;
    boost::asio::ip::tcp::socket _socket;
    boost::asio::steady_timer    _check_timer;   // 연결 유지 점검 타이머(세션 strand)
    std::chrono::steady_clock::time_point _last_recv;       // 마지막으로 패킷을 끝까지 받은 시각(strand 전용)
    std::chrono::steady_clock::time_point _unauth_since;    // Unauthenticated 상태가 시작된 시각(strand 전용, 점검 시 갱신)

    MessageQueue& _queue;
    SessionManager& _session_manager;
    ConnectionLimiter& _limiter;
    ClientAddress   _client_address;

    char                    _header_buf[HEADER_SIZE];
    std::string             _body_buf;   // 수신 body(최대 MAX_REQUEST_SIZE). 처리 큐로 이동되므로 패킷마다 새로 할당
    PacketHeader            _current_header;

    // 송신 관련 멤버는 모두 strand 전용
    std::deque<std::shared_ptr<const std::string>>  _write_queue;    // 보낼 차례를 기다리는 패킷
    std::vector<std::shared_ptr<const std::string>> _in_flight;      // 지금 전송 중인 묶음(완료까지 버퍼 유지)
    std::vector<boost::asio::const_buffer>          _write_buffers;  // _in_flight를 가리키는 버퍼 목록
    size_t _pending_bytes = 0;                                       // 대기 + 전송 중 바이트(상한 판정용)

    uint32_t    _current_room_id;
    uint32_t    _lobby_channel = 0;   // GameLoop 스레드 전용
    std::string _session_id;
    std::string _user_id;
    std::string _user_name;
    std::atomic<SessionState> _state{ SessionState::Unauthenticated };
    std::atomic<bool> _disconnected{false};
    bool        _close_after_write = false;  // strand 전용
    uint32_t    _protocol_violations = 0;    // GameLoop 스레드 전용
    TokenBucket _packet_bucket{ PACKET_RATE_LIMIT };   // 세션당 전체 패킷 빈도(strand 전용)
    uint32_t    _rate_drops = 0;                        // 이번 점검 구간에 버린 패킷 수(strand 전용, CheckConnection에서 초기화)
    // 요청 종류별 빈도(GameLoop 스레드 전용, [RequestLimit])
    std::array<TokenBucket, REQUEST_LIMIT_COUNT> _request_buckets =
        MakeRequestBuckets(std::make_index_sequence<REQUEST_LIMIT_COUNT>{});
};

// 응답 메시지에 status_code만 담아 전송(요청과 같은 PacketType). 오류/거부 응답 공용
template<typename Response>
void SendStatus(const SessionPtr& session, protobuf::PacketType type, protobuf::StatusCode code)
{
    session->SendPacket(PacketUtils::SerializeStatus<Response>(type, code));
}
