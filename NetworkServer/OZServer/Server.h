#pragma once
#include "stdafx.h"
#include "Session/Session.h"
#include "Room/RoomManager.h"
#include "Message/MessageQueue.h"
#include "Network/ConnectionLimiter.h"
#include "Lobby/LobbyManager.h"

class SessionManager;

class Server {
public:
    Server(boost::asio::io_context& io_context, SessionManager& session_manager, LobbyManager& lobby);

    // 포트를 열고(bind/listen) accept 시작. 포트가 이미 사용 중(다른 서버 실행 중 등)이면 오류 로그 후 false
    bool Start(uint16_t port);

    MessageQueue& GetMessageQueue() { return _message_queue; }
    RoomManager&  GetRoomManager()  { return _room_manager; }
    ConnectionLimiter& GetConnectionLimiter() { return _connection_limiter; }

    // 종료 1단계: 신규 접속만 차단(기존 세션 유지). 어느 스레드에서 호출해도 acceptor strand로 넘겨 처리
    void StopAccepting();
    // 종료 마지막 단계: 신규 접속 차단 + 모든 세션 종료(중복 호출 안전)
    void Stop();

private:
    void StartAccept();
    void HandleAccept(SessionPtr session, const boost::system::error_code& error);
    // 동시 접속 상한 확인 후 주소 기록. 넘으면 응답 없이 소켓을 닫고 false
    bool AdmitConnection(const SessionPtr& session);
    void StopAcceptingOnStrand();
    void RetryAcceptLater(const boost::system::error_code& error);

    // accept 오류(소켓/핸들 고갈 등)가 나면 이 간격 뒤에 재시도(즉시 재시도로 CPU를 헛쓰지 않도록)
    static constexpr auto ACCEPT_RETRY_DELAY = std::chrono::seconds(5);

    boost::asio::io_context& _io_context;
    // accept 루프와 Stop()이 서로 다른 I/O 스레드에서 acceptor를 동시에 만지지 않도록 전용 strand 위에 생성
    boost::asio::strand<boost::asio::io_context::executor_type> _strand;
    boost::asio::ip::tcp::tcp::acceptor _acceptor;
    boost::asio::steady_timer _accept_retry_timer;   // _strand에서만 사용

    bool _stopping = false;  // _strand에서만 접근

    MessageQueue _message_queue;
    ConnectionLimiter _connection_limiter{ MAX_SESSIONS, MAX_SESSIONS_PER_IP };   // 세션(I/O 스레드)이 해제 시 참조
    RoomManager  _room_manager;
    SessionManager& _session_manager;

};
