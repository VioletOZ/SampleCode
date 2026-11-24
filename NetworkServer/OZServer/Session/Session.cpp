#include "stdafx.h"
#include "Session/Session.h"
#include "Session/SessionManager.h"
#include "Common/Log.h"

void Session::Start()
{
    _session_id = boost::uuids::to_string(boost::uuids::random_generator()());
    LogDebug("Connect Session UUID : ", _session_id);
    _session_manager.AddSession(shared_from_this());
    // accept 핸들러는 acceptor strand에서 실행되므로, 소켓 작업은 세션 strand로 넘겨 시작
    boost::asio::post(_strand, [self = shared_from_this()] {
        if (self->_disconnected) return;   // 시작 전에 종료(Server::Stop)된 경우 타이머를 걸지 않음
        self->_last_recv = self->_unauth_since = std::chrono::steady_clock::now();
        self->ReadHeader();
        self->ScheduleConnectionCheck();
    });
}

void Session::ScheduleConnectionCheck()
{
    _check_timer.expires_after(CONNECTION_CHECK_INTERVAL);
    // 대기 중에는 세션을 붙잡음. HandleDisconnect가 타이머를 취소하면 operation_aborted로 끝나 세션이 해제됨
    _check_timer.async_wait(boost::asio::bind_executor(_strand,
        [self = shared_from_this()](const boost::system::error_code& error) {
            // 종료 대기 중이면 마감 타이머(ScheduleCloseDeadline)를 덮어쓰지 않도록 아무것도 하지 않음
            if (error || self->_disconnected || self->_close_after_write) return;
            self->CheckConnection();
        }));
}

void Session::ScheduleCloseDeadline()
{
    _check_timer.expires_after(CLOSE_WRITE_TIMEOUT);   // 대기 중이던 연결 점검은 취소됨(operation_aborted)
    _check_timer.async_wait(boost::asio::bind_executor(_strand,
        [self = shared_from_this()](const boost::system::error_code& error) {
            if (error || self->_disconnected) return;   // 전송이 끝나 이미 종료됨(타이머 취소)
            LogDebug("Session ", self->_session_id, " close timeout (pending writes not completed), disconnecting");
            self->HandleDisconnect();
        }));
}

void Session::CheckConnection()
{
    const auto now = std::chrono::steady_clock::now();
    _rate_drops = 0;   // 패킷 빈도 초과 집계 구간 = 점검 주기
    const SessionState state = GetState();
    if (state != SessionState::Unauthenticated) _unauth_since = now;  // 로그인 중/완료면 로그인 지연 시간 초기화

    const ConnectionTimeout timeout = CheckConnectionTimeout(state, now - _last_recv, now - _unauth_since);
    if (timeout == ConnectionTimeout::None) {
        ScheduleConnectionCheck();
        return;
    }
    LogDebug("Session ", _session_id, (timeout == ConnectionTimeout::Idle ? " idle timeout" : " login timeout"), ", disconnecting");
    HandleDisconnect();
}

void Session::OnPacketRateExceeded()
{
    _limiter.CountPacketDropped();
    if (++_rate_drops > MAX_RATE_DROPS) {
        LogWarn("Session ", _session_id, " packet rate limit exceeded (", _rate_drops, " dropped), disconnecting");
        _limiter.CountRateDisconnected();
        HandleDisconnect();
        return;
    }
    if (_rate_drops == 1) {   // 구간마다 첫 초과만 알림(버릴 때마다 응답하면 공격 트래픽을 키움)
        protobuf::S_ProtocolError notice;
        notice.set_status_code(protobuf::ERR_RATE_LIMITED);
        notice.set_request_type(_current_header.type);
        DoSendPacket(std::make_shared<const std::string>(PacketUtils::Serialize(protobuf::PROTOCOL_ERROR, notice)));
    }
    ReadHeader();
}

void Session::SendPacket(std::string message)
{
    SendPacket(std::make_shared<const std::string>(std::move(message)));
}

void Session::SendPacket(std::shared_ptr<const std::string> message)
{
    boost::asio::post(_strand,
        [self = shared_from_this(), msg = std::move(message)]() mutable {
            self->DoSendPacket(std::move(msg));
        });
}

void Session::DoSendPacket(std::shared_ptr<const std::string> serialized)
{
    if (_disconnected) return;  // 이미 끊긴 세션에는 더 쌓지 않음

    if (IsSendQueueOverLimit(_pending_bytes, serialized->size())) {
        LogWarn("Send queue limit exceeded (", _pending_bytes, " bytes pending), disconnecting session ", _session_id);
        HandleDisconnect();
        return;
    }

    _pending_bytes += serialized->size();
    _write_queue.push_back(std::move(serialized));
    if (_in_flight.empty()) {
        Write();
    }
}

void Session::HandleDisconnect()
{
    if (_disconnected.exchange(true)) return;  // 이미 처리됨(중복 호출) 방지

    boost::system::error_code ec;
    _check_timer.cancel();

    if (_socket.is_open()) {
        _socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
        if (ec) {
            LogDebug("Error shutting down socket: ", ec.message());   // 상대가 먼저 끊은 경우 흔함
        }
    }

    // I/O 스레드에서 호출되므로 GameLoop 스레드가 쓰는 _user_id 대신 불변인 세션 ID 사용
    LogDebug("Session ", _session_id, " disconnected");

    _limiter.Release(_client_address);   // 접속 수 반환(_disconnected 가드로 정확히 1회)

    // 룸/세션 정리는 GameLoop 스레드로 넘김 (RoomManager::_rooms 는 그 스레드에서만 접근)
    _queue.PushDisconnect(shared_from_this());

    _socket.close();
}
