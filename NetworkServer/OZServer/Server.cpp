#include "stdafx.h"
#include "Server.h"
#include "Session/SessionManager.h"
#include "Common/Log.h"

Server::Server(boost::asio::io_context& io_context, SessionManager& session_manager, LobbyManager& lobby)
    : _io_context(io_context), _strand(boost::asio::make_strand(io_context)),
      _acceptor(_strand), _accept_retry_timer(_strand),
      _room_manager(lobby), _session_manager(session_manager)
{
}

bool Server::Start(uint16_t port)
{
    using boost::asio::ip::tcp;
    const tcp::endpoint endpoint(tcp::v4(), port);
    boost::system::error_code ec;
    _acceptor.open(endpoint.protocol(), ec);
    // reuse_address를 끔: endpoint를 받는 acceptor 생성자는 이를 켜는데, Windows의 SO_REUSEADDR는
    // 다른 서버가 listen 중인 포트에도 bind를 허용함(두 번째 서버가 정상 시작한 것처럼 보이고 접속은 기존 서버로 감)
    if (!ec) _acceptor.set_option(tcp::acceptor::reuse_address(false), ec);
    if (!ec) _acceptor.bind(endpoint, ec);
    if (!ec) _acceptor.listen(boost::asio::socket_base::max_listen_connections, ec);
    if (ec) {
        // ec.message()는 OS 언어(한국어 Windows면 CP949)라 로그 파일에서 깨질 수 있어 코드 번호도 함께 남김(10048 = 사용 중인 주소)
        LogError("Server: cannot listen on port ", port, " (error ", ec.value(), "): ", ec.message());
        boost::system::error_code ignored;
        _acceptor.close(ignored);
        return false;
    }
    StartAccept();   // io_context.run() 전이라 다른 스레드와 겹치지 않음
    return true;
}

void Server::StartAccept()
{
    SessionPtr new_session = std::make_shared<Session>(_io_context, _message_queue, _session_manager, _connection_limiter);
    _acceptor.async_accept(new_session->socket(),
        boost::bind(&Server::HandleAccept, this, new_session, boost::asio::placeholders::error));
}

void Server::HandleAccept(SessionPtr session, const boost::system::error_code& error)
{
    // 종료 중이면 새 세션을 시작하지 않고 accept 루프도 재개하지 않음(acceptor 닫힘 -> operation_aborted)
    if (_stopping) return;

    if (error) {
        RetryAcceptLater(error);
        return;
    }
    if (AdmitConnection(session)) {
        session->Start();  // SessionManager에 등록됨(세션 추적은 SessionManager 하나로 일원화)
    }
    StartAccept();
}

bool Server::AdmitConnection(const SessionPtr& session)
{
    boost::system::error_code ec;
    const auto endpoint = session->socket().remote_endpoint(ec);
    if (ec) {   // accept 직후 상대가 이미 끊음
        session->socket().close(ec);
        return false;
    }
    ClientAddress address{ endpoint.address().to_string(), endpoint.address().is_loopback() };

    const AcquireResult result = _connection_limiter.TryAcquire(address);
    if (result != AcquireResult::Ok) {
        // 아직 패킷을 주고받기 전이므로 응답 없이 닫음. 클라이언트가 유발할 수 있으므로 Debug + [Stats] 집계
        _connection_limiter.CountRejected();
        LogDebug("Connection rejected (", (result == AcquireResult::ServerFull ? "server full" : "per-IP limit"), "): ", address.ip);
        session->socket().close(ec);
        return false;
    }
    session->SetClientAddress(std::move(address));
    return true;
}

// accept 실패: 경고를 남기고 ACCEPT_RETRY_DELAY 뒤에 다시 accept. 종료 중이면 타이머가 취소되어 재개하지 않음
void Server::RetryAcceptLater(const boost::system::error_code& error)
{
    LogWarn("Accept failed: ", error.message(), " (retry in ", ACCEPT_RETRY_DELAY.count(), "s)");
    _accept_retry_timer.expires_after(ACCEPT_RETRY_DELAY);
    _accept_retry_timer.async_wait([this](const boost::system::error_code& ec) {
        if (ec || _stopping) return;
        StartAccept();
    });
}

void Server::StopAccepting()
{
    boost::asio::post(_strand, [this] { StopAcceptingOnStrand(); });
}

void Server::StopAcceptingOnStrand()
{
    _stopping = true;

    boost::system::error_code ec;
    _acceptor.close(ec);
    _accept_retry_timer.cancel();
}

void Server::Stop()
{
    boost::asio::post(_strand, [this] {
        StopAcceptingOnStrand();
        // 소켓이 닫히면 대기 중이던 비동기 작업이 에러로 끝나고 io_context::run()이 반환됨.
        // 세션 종료는 각 세션 strand에서 실행(Close -> HandleDisconnect). 이후 GameLoop으로 disconnect 이벤트가 넘어가 정리(중복 호출 안전)
        for (const auto& session : _session_manager.GetAllSessions()) {
            session->Close();
        }
    });
}
