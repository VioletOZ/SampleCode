// Client.cpp - OZServer 접속 콘솔 클라이언트
//
// 방 생성/참가/나가기 및 채팅 메시지 전송 예제.
// 서버 접속은 /login 입력 시점에 한다(로그인 전에는 서버에 붙지 않음). 연결 중에는 30초마다 하트비트를 보내고,
// 로그아웃/로그인 실패/연결 끊김 시 연결을 닫는다(자동 재접속 없음 — 다시 /login).
// 빌드: Visual Studio 2022, x64, C++20, ws2_32.lib + protobuf(vcpkg) 링크 필요
//
// 와이어 포맷 (서버 Network/PacketHeader.h::PacketHeader 와 동일해야 함):
//   [PacketHeader: body_size(4, network byte order) + type(2, network byte order)]  = 6바이트
//   [액션별 C_Xxx(요청)/S_Xxx(응답) protobuf 메시지로 직렬화된 body]

#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>

#include "proto/UserPacket.pb.h"
#include "proto/RoomPacket.pb.h"
#include "proto/ChatPacket.pb.h"
#include "proto/LobbyPacket.pb.h"
#include "proto/GamePacket.pb.h"
#include "proto/Common.pb.h"

#pragma comment(lib, "ws2_32.lib")

static_assert(sizeof(size_t) == 8, "반드시 x64 로 빌드하세요.");

constexpr const char* SERVER_IP = "127.0.0.1";
constexpr uint16_t    SERVER_PORT = 12345;
constexpr auto        HEARTBEAT_INTERVAL = std::chrono::seconds(30);   // 서버 무응답 끊기(60초)보다 짧게
constexpr int64_t     LOGIN_RETRY_COOLDOWN_MS = 3000;    // 같은 ID 로그인 재요청 최소 간격
constexpr int64_t     LOGIN_BUSY_COOLDOWN_MS = 10000;    // ERR_SERVER_BUSY(대기열 가득) / ERR_RATE_LIMITED(시도 과다) 후 재요청 간격
constexpr size_t      MAX_CHAT_BYTES = 150;              // 서버와 동일: 한글 기준 50자(UTF-8 3바이트/자)

// 서버 Network/PacketHeader.h::PacketHeader 와 동일한 레이아웃
#pragma pack(push, 1)
struct PacketHeader {
    uint32_t body_size;
    uint16_t type;
};
#pragma pack(pop)

static constexpr size_t HEADER_SIZE = sizeof(PacketHeader);

enum class LoginState : uint8_t { LoggedOut, Pending, LoggedIn };

static SOCKET           g_sock = INVALID_SOCKET;   // 대입/닫기/송신은 g_send_mutex 안에서
static std::mutex       g_send_mutex;              // 입력 스레드와 하트비트 스레드의 송신 직렬화
static std::atomic_bool g_running{ true };     // false 가 되면 프로그램 전체 종료 (/quit)
static std::atomic_bool g_connected{ false };  // 현재 세션이 연결되어 있는지
static std::atomic<uint32_t>   g_room_id{ 0 };
static std::atomic<uint32_t>   g_channel{ 0 };            // 내 로비 채널(로그인 응답 / 채널 이동 결과로 갱신)
// 위치가 바뀌는 요청(방 생성/입장/퇴장, 채널 이동, 로그아웃)의 응답 대기 중. 응답 또는 연결 종료 시 해제
static std::atomic_bool        g_place_pending{ false };
static std::atomic<LoginState> g_login_state{ LoginState::LoggedOut };
static std::atomic<int64_t>    g_login_retry_at_ms{ 0 };  // 이 시각(NowMs) 전에는 같은 ID 로그인 재요청 안 함
static std::string             g_last_login_id;           // 입력 스레드 전용
// 방 게임: 현재 단계(S_GameState로 갱신, 방을 나가거나 연결이 끊기면 대기), 내 닉네임(스냅샷에서 내 상태 찾기 — 수신 스레드 전용),
// 마지막으로 보낸 이동 번호, 마지막으로 본 내 점수(수신 스레드 전용)
static std::atomic<int>        g_game_phase{ protobuf::GAME_WAITING };
static std::string             g_my_name;
static std::atomic<uint32_t>   g_move_seq{ 0 };
static uint32_t                g_my_score = 0;
static uint32_t                g_my_player_id = 0;        // 전체 스냅샷에서 이름으로 찾음(수신 스레드 전용)
static std::thread             g_recv_thread;             // 연결마다 시작, 다음 연결/종료 시 join
static std::mutex              g_heartbeat_mutex;
static std::condition_variable g_heartbeat_cv;

static int64_t NowMs()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
}

// 콘솔 입력(콘솔 코드 페이지, 한국어 Windows 기본 CP949)을 UTF-8로 변환. proto3 string은 UTF-8이어야 서버가 파싱함
static std::string ToUtf8(const std::string& input)
{
    const UINT code_page = GetConsoleCP();
    if (input.empty() || code_page == CP_UTF8) return input;

    const int wide_len = MultiByteToWideChar(code_page, 0, input.data(), static_cast<int>(input.size()), nullptr, 0);
    std::wstring wide(wide_len, L'\0');
    MultiByteToWideChar(code_page, 0, input.data(), static_cast<int>(input.size()), wide.data(), wide_len);

    const int utf8_len = WideCharToMultiByte(CP_UTF8, 0, wide.data(), wide_len, nullptr, 0, nullptr, nullptr);
    std::string utf8(utf8_len, '\0');
    WideCharToMultiByte(CP_UTF8, 0, wide.data(), wide_len, utf8.data(), utf8_len, nullptr, nullptr);
    return utf8;
}

// TCP 는 스트림이므로 size 바이트를 정확히 수신하는 보조 함수
static bool RecvAll(char* buf, int size)
{
    int total = 0;
    while (total < size) {
        int n = recv(g_sock, buf + total, size - total, 0);
        if (n <= 0) return false;
        total += n;
    }
    return true;
}

// size 바이트를 정확히 송신하는 보조 함수 (send()는 부분 전송될 수 있음)
static bool SendAll(const char* buf, int size)
{
    int total = 0;
    while (total < size) {
        int n = send(g_sock, buf + total, size - total, 0);
        if (n <= 0) return false;
        total += n;
    }
    return true;
}

// 공용 송신 헬퍼: 메시지를 직렬화해서 [헤더+바디]로 전송 (액션별 함수들이 이걸 공유)
template<typename ProtoMsg>
static bool SendPacket(protobuf::PacketType type, const ProtoMsg& request)
{
    std::string body;
    if (!request.SerializeToString(&body)) {
        std::cerr << "[직렬화 실패] type=" << static_cast<uint16_t>(type) << "\n";
        return false;
    }

    PacketHeader header;
    header.body_size = htonl(static_cast<uint32_t>(body.size()));
    header.type = htons(static_cast<uint16_t>(type));

    std::string packet;
    packet.resize(HEADER_SIZE + body.size());
    memcpy(packet.data(), &header, HEADER_SIZE);
    memcpy(packet.data() + HEADER_SIZE, body.data(), body.size());

    std::lock_guard<std::mutex> lock(g_send_mutex);
    if (!g_connected || g_sock == INVALID_SOCKET) {
        std::cerr << "[전송 실패] 서버에 연결되어 있지 않습니다.\n";
        return false;
    }
    if (!SendAll(packet.data(), static_cast<int>(packet.size()))) {
        std::cerr << "[송신 실패] type=" << static_cast<uint16_t>(type) << "\n";
        return false;
    }
    return true;
}

static bool SendLogin(const std::string& uuid)
{
    protobuf::C_Login request;
    request.set_uuid(uuid);
    return SendPacket(protobuf::USER_LOGIN, request);
}

static bool SendLogout()
{
    protobuf::C_Logout request;
    return SendPacket(protobuf::USER_LOGOUT, request);
}

static bool SendRoomCreate()
{
    protobuf::C_RoomCreate request;
    request.set_room_type(protobuf::ROOM_PUBLIC);
    return SendPacket(protobuf::ROOM_CREATE, request);
}

static bool SendRoomJoin(uint32_t room_id)
{
    protobuf::C_RoomJoin request;
    request.set_room_id(room_id);
    return SendPacket(protobuf::ROOM_JOIN, request);
}

static bool SendRoomLeave()
{
    protobuf::C_RoomLeave request;
    return SendPacket(protobuf::ROOM_LEAVE, request);
}

static bool SendRoomList(uint32_t page)
{
    protobuf::C_RoomList request;
    request.set_page(page);
    return SendPacket(protobuf::ROOM_LIST, request);
}

static bool SendChannelMove(uint32_t channel)
{
    protobuf::C_ChannelMove request;
    request.set_channel(channel);
    return SendPacket(protobuf::LOBBY_CHANNEL_MOVE, request);
}

static bool SendOnlineList()
{
    protobuf::C_UserList request;
    return SendPacket(protobuf::USER_LIST, request);
}

static bool SendUserList(uint32_t room_id)
{
    protobuf::C_RoomInfo request;
    request.mutable_room_info()->set_room_id(room_id);
    return SendPacket(protobuf::ROOM_USER_LIST, request);
}

// 채팅은 Room 계열이 아니라 C_ChatPacket으로 보냄 (로비/방 구분은 서버가 세션 상태로 판단)
static bool SendChatMsg(const std::string& text)
{
    protobuf::C_ChatPacket request;
    request.set_message(text);
    return SendPacket(protobuf::CHAT_MESSAGE, request);
}

static bool SendGameReady(bool ready)
{
    protobuf::C_GameReady request;
    request.set_ready(ready);
    return SendPacket(protobuf::GAME_READY, request);
}

static bool SendGameMove(float x, float y, uint32_t seq)
{
    protobuf::C_GameMove request;
    request.set_x(x);
    request.set_y(y);
    request.set_seq(seq);
    return SendPacket(protobuf::GAME_MOVE, request);
}

static const char* GamePhaseLabel(protobuf::GamePhase phase)
{
    switch (phase) {
    case protobuf::GAME_COUNTDOWN: return "카운트다운";
    case protobuf::GAME_PLAYING:   return "진행";
    case protobuf::GAME_RESULT:    return "결과";
    default:                       return "대기";
    }
}

// 방 게임 단계/인원 알림 출력. 결과 단계면 순위
static void PrintGameState(const protobuf::S_GameState& state)
{
    g_game_phase = state.phase();
    std::cout << "[게임 " << GamePhaseLabel(state.phase()) << "]";
    switch (state.phase()) {
    case protobuf::GAME_WAITING:
        std::cout << " 준비 " << state.ready_count() << "/" << state.player_count() << "명 (2명 이상 전원 준비하면 시작, /ready)\n";
        break;
    case protobuf::GAME_COUNTDOWN:
        std::cout << " " << (state.remaining_ms() + 999) / 1000 << "초 뒤 시작 (/unready 로 취소)\n";
        break;
    case protobuf::GAME_PLAYING:
        g_my_score = 0;
        std::cout << " " << state.remaining_ms() / 1000 << "초 동안 코인을 주우세요 (/move <x> <y>, 맵 0~100, 초당 10까지)\n";
        break;
    default:
        std::cout << " 순위\n";
        for (int i = 0; i < state.results_size(); ++i) {
            std::cout << "  " << i + 1 << ". " << state.results(i).name() << "  " << state.results(i).score() << "점\n";
        }
        break;
    }
}

// 스냅샷은 초당 10번 올 수 있으므로 내 정보가 바뀐 것만 출력(전체 스냅샷: 시작 위치, 이동 반영, 점수 변화)
// 출력할 것이 없으면 빈 문자열
static std::string FormatGameSnapshot(const protobuf::S_GameSnapshot& snapshot)
{
    std::ostringstream out;
    for (const auto& player : snapshot.players()) {
        if (snapshot.full()) {   // 시작: 이름으로 내 player_id를 찾음
            if (player.name() != g_my_name) continue;
            g_my_player_id = player.player_id();
            out << "[게임 시작 위치] (" << player.x() << ", " << player.y() << ")  코인 " << snapshot.coins_size() << "개:";
            for (const auto& coin : snapshot.coins()) out << " (" << coin.x() << ", " << coin.y() << ")";
            out << "\n";
            continue;
        }
        if (player.player_id() != g_my_player_id) continue;
        if (player.last_seq() == g_move_seq) out << "[이동 반영] (" << player.x() << ", " << player.y() << ")\n";
        if (player.score() != g_my_score) {
            g_my_score = player.score();
            out << "[코인 획득] 점수 " << g_my_score << "\n";
        }
    }
    if (!snapshot.full() && snapshot.coins_size() > 0) {
        out << "[새 코인]";
        for (const auto& coin : snapshot.coins()) out << " (" << coin.x() << ", " << coin.y() << ")";
        out << "\n";
    }
    return out.str();
}

static bool ConnectToServer()
{
    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) {
        std::cerr << "socket() 실패: " << WSAGetLastError() << "\n";
        return false;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, SERVER_IP, &addr.sin_addr);

    if (connect(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0) {
        closesocket(sock);
        return false;
    }
    std::lock_guard<std::mutex> lock(g_send_mutex);
    g_sock = sock;
    return true;
}

// 수신 루프가 끝난 이유(연결을 닫은 뒤 안내 문구에 사용)
enum class CloseReason { ServerClosed, LoggedOut, LoginFailed };

// 연결되어 있는 동안 서버 응답을 수신 (블로킹 recv). 연결이 끊어지거나, 로그아웃/로그인 실패로 클라이언트가 닫을 때 반환.
// 요청 실패 출력(공용): "[label 실패] ERR_이름 (status_code=n)" + 자주 나오는 코드는 안내 문구
static void PrintFailure(const char* label, protobuf::StatusCode code)
{
    const char* hint = "";
    if (code == protobuf::ERR_RATE_LIMITED)  hint = " — 너무 자주 요청했습니다. 잠시 후 다시 입력하세요";
    if (code == protobuf::ERR_INVALID_STATE) hint = " — 지금 상태에서 할 수 없는 요청입니다";
    std::cout << "[" << label << " 실패] " << protobuf::StatusCode_Name(code) << " (status_code=" << code << ")" << hint << "\n";
}

static CloseReason RecvLoop()
{
    while (true) {
        char header_buf[HEADER_SIZE];
        if (!RecvAll(header_buf, static_cast<int>(HEADER_SIZE))) {
            return CloseReason::ServerClosed;
        }

        PacketHeader header;
        memcpy(&header, header_buf, HEADER_SIZE);
        header.body_size = ntohl(header.body_size);
        header.type = ntohs(header.type);

        std::string body(header.body_size, '\0');
        if (header.body_size > 0 && !RecvAll(body.data(), static_cast<int>(header.body_size))) {
            return CloseReason::ServerClosed;
        }

        // 스냅샷은 초당 최대 10번 오므로 출력할 것이 있을 때만 줄바꿈·프롬프트를 출력(콘솔 도배 방지)
        if (header.type == protobuf::GAME_SNAPSHOT) {
            protobuf::S_GameSnapshot snapshot;
            const std::string text = snapshot.ParseFromString(body) ? FormatGameSnapshot(snapshot) : "[응답 파싱 실패] type=GAME_SNAPSHOT\n";
            if (!text.empty()) std::cout << "\n" << text << "> " << std::flush;
            continue;
        }

        std::cout << "\n";
        switch (static_cast<protobuf::PacketType>(header.type)) {
        case protobuf::USER_LOGIN: {
            protobuf::S_Login response;
            if (!response.ParseFromString(body)) {
                std::cerr << "[응답 파싱 실패] type=USER_LOGIN\n";
                break;
            }
            if (response.status_code() == protobuf::OK) {
                g_login_state = LoginState::LoggedIn;
                g_channel = response.channel();
                g_room_id = 0;   // 로그인 직후는 항상 로비(연결이 끊기면 방에서도 나감)
                g_my_name = response.user_info().name();
                std::cout << "[로그인 성공] id=" << response.user_info().id()
                    << "  nickname=" << response.user_info().name() << "  채널 " << response.channel() << "\n";
                break;
            }
            if (response.status_code() == protobuf::ERR_SERVER_BUSY || response.status_code() == protobuf::ERR_RATE_LIMITED) {
                g_login_retry_at_ms = NowMs() + LOGIN_BUSY_COOLDOWN_MS;
            }
            std::cout << "[로그인 실패] " << protobuf::StatusCode_Name(response.status_code())
                << " (status_code=" << response.status_code() << ")\n";
            return CloseReason::LoginFailed;  // 로그인 전에는 서버에 붙어 있지 않음 → 연결 종료
        }
        case protobuf::LOGIN_QUEUE: {
            // 로그인 요청이 몰려 서버 대기열에 있음. 차례가 되면 USER_LOGIN(S_Login)으로 결과가 옴
            protobuf::S_LoginQueue notice;
            if (!notice.ParseFromString(body)) {
                std::cerr << "[응답 파싱 실패] type=LOGIN_QUEUE\n";
                break;
            }
            std::cout << "[로그인 대기] 순번 " << notice.position() << " / 대기 " << notice.waiting() << "명\n";
            break;
        }
        case protobuf::USER_LOGOUT: {
            protobuf::S_Logout response;
            if (!response.ParseFromString(body)) {
                std::cerr << "[응답 파싱 실패] type=USER_LOGOUT\n";
                break;
            }
            if (response.status_code() == protobuf::OK) {
                std::cout << "[로그아웃 완료]\n";
                return CloseReason::LoggedOut;  // 로그아웃 후에는 연결을 닫음(다시 /login 시 재접속)
            }
            else {
                g_place_pending = false;
                PrintFailure("로그아웃", response.status_code());
            }
            break;
        }
        case protobuf::CHAT_MESSAGE: {
            // 서버 에러 응답(S_ChatPacket)과 브로드캐스트(C_ChatPacket)는 status_code가 같은 필드 번호(1)라 함께 파싱 가능
            protobuf::C_ChatPacket chat;
            if (!chat.ParseFromString(body)) {
                std::cerr << "[채팅 파싱 실패]\n";
                break;
            }
            if (chat.status_code() != protobuf::OK) {
                PrintFailure("채팅", static_cast<protobuf::StatusCode>(chat.status_code()));   // ChatPacket의 status_code는 int32
                break;
            }
            const char* tag = (chat.chat_type() == protobuf::CHAT_ROOM) ? "[방]" : "[로비]";
            std::cout << tag << " " << chat.user_info().name() << ": " << chat.message() << "\n";
            break;
        }
        case protobuf::ROOM_CREATE: {
            protobuf::S_RoomCreate response;
            if (!response.ParseFromString(body)) {
                std::cerr << "[응답 파싱 실패] type=ROOM_CREATE\n";
                break;
            }
            g_place_pending = false;
            if (response.status_code() != protobuf::OK) {
                PrintFailure("방 생성", response.status_code());
                break;
            }
            g_room_id = response.room_info().room_id();   // 방 번호는 서버 성공 응답에서만 바꿈
            std::cout << "[방 생성] room_id=" << response.room_info().room_id()
                << "  room_name=" << response.room_info().room_name() << "\n";
            break;
        }
        case protobuf::ROOM_JOIN: {
            protobuf::S_RoomJoin response;
            if (!response.ParseFromString(body)) {
                std::cerr << "[응답 파싱 실패] type=ROOM_JOIN\n";
                break;
            }
            g_place_pending = false;
            if (response.status_code() != protobuf::OK) {
                PrintFailure("방 입장", response.status_code());
                break;
            }
            g_room_id = response.room_info().room_id();
            std::cout << "[방 입장] room_id=" << response.room_info().room_id()
                << "  room_name=" << response.room_info().room_name() << "\n";
            break;
        }
        case protobuf::ROOM_LEAVE: {
            protobuf::S_RoomLeave response;
            if (!response.ParseFromString(body)) {
                std::cerr << "[응답 파싱 실패] type=ROOM_LEAVE\n";
                break;
            }
            g_place_pending = false;
            if (response.status_code() == protobuf::OK || response.status_code() == protobuf::ERR_NOT_IN_ROOM) {
                g_room_id = 0;   // 서버 기준으로 방 밖(ERR_NOT_IN_ROOM이면 클라이언트 상태만 어긋났던 것)
                g_game_phase = protobuf::GAME_WAITING;
            }
            if (response.status_code() != protobuf::OK) {
                PrintFailure("방 퇴장", response.status_code());
                break;
            }
            std::cout << "[방 퇴장]\n";
            break;
        }
        case protobuf::ROOM_LIST: {
            protobuf::S_RoomList response;
            if (!response.ParseFromString(body)) {
                std::cerr << "[응답 파싱 실패] type=ROOM_LIST\n";
                break;
            }
            if (response.status_code() != protobuf::OK) {
                PrintFailure("방 목록", response.status_code());
                break;
            }
            if (response.total_rooms() == 0) {
                std::cout << "[방 목록] 방이 없습니다\n";
                break;
            }
            std::cout << "[방 목록] " << response.page() << "/" << response.total_pages() << " 페이지 (전체 "
                << response.total_rooms() << "개)\n";
            for (const auto& room_info : response.room_list()) {
                std::cout << "  - room_id=" << room_info.room_id() << "  room_name=" << room_info.room_name() << "\n";
            }
            if (response.page() < response.total_pages()) {
                std::cout << "  다음 페이지: /rooms " << response.page() + 1 << "\n";
            }
            break;
        }
        case protobuf::ROOM_USER_LIST: {
            protobuf::S_RoomInfo response;
            if (!response.ParseFromString(body)) {
                std::cerr << "[응답 파싱 실패] type=ROOM_USER_LIST\n";
                break;
            }
            if (response.status_code() != protobuf::OK) {
                PrintFailure("유저 목록", response.status_code());
                break;
            }
            if (response.room_info().room_id() == 0) {
                std::cout << "[로비 유저 목록] 채널 " << response.channel() << " / 총 " << response.user_list_size() << "명\n";
            }
            else {
                std::cout << "[방 유저 목록] " << response.room_info().room_name() << " (room_id="
                    << response.room_info().room_id() << ")\n";
            }
            for (const auto& user : response.user_list()) {
                std::cout << "  - " << user.name() << "\n";
            }
            break;
        }
        case protobuf::USER_LIST: {
            protobuf::S_UserList response;
            if (!response.ParseFromString(body)) {
                std::cerr << "[응답 파싱 실패] type=USER_LIST\n";
                break;
            }
            if (response.status_code() != protobuf::OK) {
                PrintFailure("접속자 목록", response.status_code());
                break;
            }
            std::cout << "[전체 접속자] 총 " << response.total() << "명\n"
                << "  - 로비: " << response.lobby() << "명\n"
                << "  - 방: " << response.in_room() << "명\n";
            break;
        }
        case protobuf::LOBBY_CHANNEL_MOVE: {
            protobuf::S_ChannelMove response;
            if (!response.ParseFromString(body)) {
                std::cerr << "[응답 파싱 실패] type=LOBBY_CHANNEL_MOVE\n";
                break;
            }
            g_place_pending = false;
            if (response.channel() != 0) g_channel = response.channel();   // ERR_INVALID_STATE 응답은 채널 값이 없음
            switch (response.status_code()) {
            case protobuf::OK:
                std::cout << "[채널 이동] 채널 " << response.channel() << "\n";
                break;
            case protobuf::ERR_CHANNEL_NOT_FOUND:
                std::cout << "[채널 이동 실패] 없는 채널입니다. 지금은 1~" << response.channel_count() << " 채널이 있습니다\n";
                break;
            case protobuf::ERR_CHANNEL_FULL:
                std::cout << "[채널 이동 실패] 채널 정원(100명)이 찼습니다\n";
                break;
            case protobuf::ERR_SAME_CHANNEL:
                std::cout << "[채널 이동 실패] 이미 채널 " << response.channel() << "에 있습니다\n";
                break;
            case protobuf::ERR_INVALID_STATE:
                std::cout << "[채널 이동 실패] 로비에서만 이동할 수 있습니다(방에서 /leave 후 이동)\n";
                break;
            default:
                std::cout << "[채널 이동 실패] " << protobuf::StatusCode_Name(response.status_code())
                    << " (status_code=" << response.status_code() << ")\n";
                break;
            }
            break;
        }
        case protobuf::GAME_READY: {
            protobuf::S_GameReady response;
            if (!response.ParseFromString(body)) {
                std::cerr << "[응답 파싱 실패] type=GAME_READY\n";
                break;
            }
            if (response.status_code() != protobuf::OK) {
                PrintFailure(response.ready() ? "준비" : "준비 취소", response.status_code());
                break;
            }
            std::cout << (response.ready() ? "[준비 완료]\n" : "[준비 취소]\n");
            break;
        }
        case protobuf::GAME_STATE: {
            protobuf::S_GameState state;
            if (!state.ParseFromString(body)) {
                std::cerr << "[응답 파싱 실패] type=GAME_STATE\n";
                break;
            }
            PrintGameState(state);
            break;
        }
        case protobuf::GAME_MOVE: {
            // 받아들인 이동은 응답 없음(스냅샷으로 반영). 거부 시에만 서버 위치가 옴
            protobuf::S_GameMove response;
            if (!response.ParseFromString(body)) {
                std::cerr << "[응답 파싱 실패] type=GAME_MOVE\n";
                break;
            }
            if (response.status_code() == protobuf::ERR_INVALID_MOVE) {
                std::cout << "[이동 거부] 너무 멀거나 맵 밖입니다. 서버 위치 (" << response.x() << ", " << response.y()
                    << ") — 한 번에 5까지, 초당 10까지 이동\n";
                break;
            }
            PrintFailure("이동", response.status_code());
            break;
        }
        case protobuf::PROTOCOL_ERROR: {
            // 서버가 이해하지 못한 요청(미등록 타입/손상된 패킷, 3회 누적 시 연결 끊김) 또는
            // 요청 빈도 초과(ERR_RATE_LIMITED: 초과분은 버려지고, 계속 넘기면 연결 끊김)
            protobuf::S_ProtocolError response;
            if (!response.ParseFromString(body)) {
                std::cerr << "[응답 파싱 실패] type=PROTOCOL_ERROR\n";
                break;
            }
            std::cout << "[프로토콜 오류] 요청 type=" << response.request_type()
                << "  " << protobuf::StatusCode_Name(response.status_code()) << "\n";
            break;
        }
        default:
            std::cout << "[수신] type=" << header.type << " (알 수 없는 타입)\n";
            break;
        }
        std::cout << "> " << std::flush;
    }
}

// 연결마다 1개: 수신 루프가 끝나면(서버 종료/로그아웃/로그인 실패) 소켓을 닫고 로그아웃 상태로 되돌림
static void RecvThread()
{
    const CloseReason reason = RecvLoop();
    {
        std::lock_guard<std::mutex> lock(g_send_mutex);
        g_connected = false;
        closesocket(g_sock);
        g_sock = INVALID_SOCKET;
    }
    g_login_state = LoginState::LoggedOut;
    g_room_id = 0;
    g_game_phase = protobuf::GAME_WAITING;
    g_place_pending = false;
    if (!g_running) return;

    switch (reason) {
    case CloseReason::LoggedOut:   std::cout << "[연결 종료] /login <id> 로 다시 접속할 수 있습니다.\n"; break;
    case CloseReason::LoginFailed: std::cout << "[연결 종료] 잠시 후 /login <id> 로 다시 시도하세요.\n"; break;
    case CloseReason::ServerClosed:std::cout << "\n[서버 연결이 끊어졌습니다] /login <id> 로 다시 접속하세요.\n"; break;
    }
    std::cout << "> " << std::flush;
}

// 연결되어 있지 않으면 접속하고 수신 스레드 시작(로그인할 때만 호출 → 로그인 전에는 서버에 붙지 않음)
static bool EnsureConnected()
{
    if (g_connected) return true;
    if (g_recv_thread.joinable()) g_recv_thread.join();  // 이전 연결의 수신 스레드(이미 끝남) 정리

    std::cout << "서버 연결 시도: " << SERVER_IP << ":" << SERVER_PORT << " ...\n";
    if (!ConnectToServer()) {
        std::cout << "[연결 실패] 서버 실행 여부를 확인한 뒤 다시 /login 하세요.\n";
        return false;
    }
    g_connected = true;
    g_recv_thread = std::thread(RecvThread);
    std::cout << "[서버 연결 성공]\n";
    return true;
}

// 연결 중이면 HEARTBEAT_INTERVAL마다 C_Heartbeat 전송(서버는 60초 무응답 시 연결을 끊음). /quit 시 즉시 깨어나 종료
static void HeartbeatThread()
{
    std::unique_lock<std::mutex> lock(g_heartbeat_mutex);
    while (g_running) {
        g_heartbeat_cv.wait_for(lock, HEARTBEAT_INTERVAL, [] { return !g_running; });
        if (g_running && g_connected) SendPacket(protobuf::HEARTBEAT, protobuf::C_Heartbeat{});
    }
}

// /login 처리: 로그인 중/로그인됨이면 보내지 않고, 같은 ID는 재요청 간격(3초, 대기열 가득 후 10초)이 지나야 전송
static void RequestLogin(const std::string& uuid)
{
    const LoginState state = g_login_state;
    if (state == LoginState::Pending) {
        std::cout << "[로그인 처리 중] 결과를 기다리는 중입니다.\n";
        return;
    }
    if (state == LoginState::LoggedIn) {
        std::cout << "[이미 로그인됨] 다른 ID로 로그인하려면 /logout 후 다시 /login 하세요.\n";
        return;
    }
    const int64_t wait_ms = g_login_retry_at_ms - NowMs();
    if (uuid == g_last_login_id && wait_ms > 0) {
        std::cout << "[잠시 후 다시 시도] " << (wait_ms + 999) / 1000 << "초 남음\n";
        return;
    }
    if (!EnsureConnected()) return;

    g_last_login_id = uuid;
    g_login_retry_at_ms = NowMs() + LOGIN_RETRY_COOLDOWN_MS;
    g_login_state = LoginState::Pending;
    std::cout << "[로그인 요청 전송] id=" << uuid << "\n";
    if (!SendLogin(uuid)) g_login_state = LoginState::LoggedOut;
}

// ---- 보낼 수 없는 요청은 서버로 보내지 않음(서버도 검사하지만 불필요한 요청/응답 왕복을 줄임) ----

// 명령을 쓸 수 있는 위치. 서버 허용 상태(OZServer GameLoop::RegisterHandlers의 등록 줄)와 같게 유지
enum class Place : uint8_t { Lobby, Room, Any };

// 로그인 후 명령을 지금 보낼 수 있는지 확인. 안 되면 이유를 출력하고 false(입력 스레드 전용)
//   changes_place: 위치가 바뀌는 요청(방 생성/입장/퇴장, 채널 이동, 로그아웃) — 이전 요청 응답 대기 중이면 막음
static bool CanRequest(Place place, bool changes_place)
{
    if (changes_place && g_place_pending) {
        std::cout << "[처리 중] 이전 요청(방 생성/입장/퇴장, 채널 이동, 로그아웃)의 결과를 기다리는 중입니다.\n";
        return false;
    }
    const bool in_room = g_room_id != 0;
    if (place == Place::Lobby && in_room) {
        std::cout << "[사용 불가] 방 안에서는 사용할 수 없습니다. /leave 로 방에서 나간 뒤 사용하세요.\n";
        return false;
    }
    if (place == Place::Room && !in_room) {
        std::cout << "[사용 불가] 방 안에서만 사용할 수 있습니다.\n";
        return false;
    }
    return true;
}

// 위치가 바뀌는 요청 전송: 응답이 올 때까지 다음 위치 변경 요청을 막음(전송 실패면 바로 해제)
template <typename SendFn>
static void SendPlaceRequest(SendFn send)
{
    g_place_pending = true;
    if (!send()) g_place_pending = false;
}

// 서버 레이트리밋과 같은 수치의 토큰 버킷(넘을 요청은 보내지 않음). 입력 스레드 전용.
// 서버 값(PacketPolicy.h CHAT_RATE_LIMIT / LIST_RATE_LIMIT)이 바뀌면 같이 바꿔야 함
struct LocalRateLimit {
    double burst;
    double per_sec;
    double tokens;
    std::chrono::steady_clock::time_point last = std::chrono::steady_clock::now();

    LocalRateLimit(double burst_, double per_sec_) : burst(burst_), per_sec(per_sec_), tokens(burst_) {}

    bool TryTake()
    {
        const auto now = std::chrono::steady_clock::now();
        tokens = (std::min)(burst, tokens + std::chrono::duration<double>(now - last).count() * per_sec);
        last = now;
        if (tokens < 1.0) return false;
        tokens -= 1.0;
        return true;
    }
};
static LocalRateLimit g_chat_limit{ 5, 2 };        // 채팅: 연속 5회, 초당 2회
static LocalRateLimit g_room_list_limit{ 3, 1 };   // 방 목록: 연속 3회, 초당 1회

static void PrintHelp()
{
    std::cout <<
        "==========================\n"
        " OZServer 클라이언트\n"
        "==========================\n"
        " /login <id>     서버 접속 + 로그인 (없는 id면 자동 생성). 로그인 전에는 아래 명령/채팅 불가\n"
        " /create         방 생성 (로비)\n"
        " /join <id>      방 참가 (로비, 예: /join 1234)\n"
        " /leave          방 나가기 (방 안)\n"
        " /rooms [페이지]  방 목록 (로비, 10개씩 방 번호 순. 예: /rooms 2)\n"
        " /channel [번호]  로비 채널 이동 (로비, 예: /channel 2. 번호 없으면 현재 채널 표시)\n"
        " /users [방번호]  유저 목록 (로비: 같은 채널의 로비 유저, 방 안: 내 방, 방번호 지정 시 해당 방)\n"
        " /all            전체 접속자 수 (로비 / 방 인원)\n"
        " /ready          게임 준비 (방 안, 2명 이상 전원 준비하면 3초 뒤 시작) / /unready 준비 취소\n"
        " /move <x> <y>   게임 중 이동 (맵 0~100, 한 번에 5까지·초당 10까지 — 넘으면 서버가 거부하고 위치를 돌려줌)\n"
        " /logout         로그아웃 후 연결 종료 (다른 id는 /login 으로 다시 접속)\n"
        " <내용>          (슬래시 없이 입력) 로비에서는 같은 채널 로비 채팅, 방 안에서는 방 채팅 (한글 50자까지)\n"
        " /help           도움말\n"
        " /quit           종료\n"
        " ※ 지금 위치(로비/방)에서 쓸 수 없는 명령, 이전 요청 응답 대기 중인 이동 요청, 빈도 초과 요청은 서버로 보내지 않음\n"
        "==========================\n\n";
}

int main()
{
    SetConsoleOutputCP(CP_UTF8);

    WSADATA wsa{};
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        std::cerr << "WSAStartup 실패\n";
        return 1;
    }

    PrintHelp();

    std::thread heartbeat_thread(HeartbeatThread);

    std::string line;
    while (g_running) {
        std::cout << "> " << std::flush;
        if (!std::getline(std::cin, line))
            break;
        if (line.empty())
            continue;

        if (line == "/quit") {
            g_running = false;

        }
        else if (line == "/help") {
            PrintHelp();

        }
        else if (line.rfind("/login", 0) == 0) {
            const std::string uuid = (line.size() > 7) ? ToUtf8(line.substr(7)) : "";
            if (uuid.empty()) {
                std::cout << "사용법: /login <id>\n";
            }
            else {
                RequestLogin(uuid);
            }

        }
        else if (g_login_state != LoginState::LoggedIn) {
            // 로그인 전/로그인 중에는 서버에 요청을 보내지 않음(서버도 FSM으로 거부하지만 불필요한 왕복 차단)
            std::cout << (g_login_state == LoginState::Pending
                ? "[로그인 처리 중] 로그인 결과를 받은 뒤 사용할 수 있습니다.\n"
                : "[로그인 필요] /login <id> 를 먼저 입력하세요.\n");

        }
        else if (line == "/create") {
            if (CanRequest(Place::Lobby, true)) {
                std::cout << "[방 생성 요청 전송]\n";
                SendPlaceRequest(SendRoomCreate);
            }

        }
        else if (line.rfind("/join", 0) == 0) {
            std::istringstream iss(line.substr(5));
            uint32_t rid = 0;
            iss >> rid;
            if (rid == 0) {
                std::cout << "사용법: /join <방번호>  (예: /join 1234)\n";
            }
            else if (CanRequest(Place::Lobby, true)) {
                std::cout << "[방 " << rid << " 참가 요청 전송]\n";   // 방 번호는 성공 응답에서 설정
                SendPlaceRequest([rid] { return SendRoomJoin(rid); });
            }

        }
        else if (line == "/leave") {
            if (CanRequest(Place::Room, true)) {
                std::cout << "[방 " << g_room_id << " 퇴장 요청 전송]\n";   // 방 번호는 성공 응답에서 0으로
                SendPlaceRequest(SendRoomLeave);
            }

        }
        else if (line.rfind("/rooms", 0) == 0) {
            std::istringstream iss(line.substr(6));
            uint32_t page = 1;
            if (!(iss >> page) || page == 0) page = 1;   // 인자가 없으면 1페이지
            if (CanRequest(Place::Lobby, false)) {
                if (!g_room_list_limit.TryTake()) {
                    std::cout << "[방 목록 제한] 초당 1회(연속 3회)까지 요청할 수 있습니다. 잠시 후 다시 입력하세요.\n";
                }
                else {
                    std::cout << "[방 목록 요청 전송] " << page << "페이지\n";
                    SendRoomList(page);
                }
            }

        }
        else if (line.rfind("/channel", 0) == 0) {
            std::istringstream iss(line.substr(8));
            uint32_t channel = 0;
            if (!(iss >> channel)) {
                std::cout << "[현재 채널] " << g_channel << " (이동: /channel <번호>)\n";
            }
            else if (channel == 0) {
                std::cout << "사용법: /channel <번호>  (채널은 1부터)\n";
            }
            else if (channel == g_channel) {
                std::cout << "[채널 이동 불가] 이미 채널 " << channel << "에 있습니다.\n";
            }
            else if (CanRequest(Place::Lobby, true)) {
                std::cout << "[채널 이동 요청 전송] 채널 " << channel << "\n";
                SendPlaceRequest([channel] { return SendChannelMove(channel); });
            }

        }
        else if (line.rfind("/users", 0) == 0) {
            std::istringstream iss(line.substr(6));
            uint32_t rid = 0;
            if (!(iss >> rid)) rid = g_room_id;  // 인자가 없으면 현재 방(로비면 0 → 서버가 로비 목록으로 처리)
            std::cout << "[유저 목록 요청 전송] room=" << rid << "\n";
            SendUserList(rid);

        }
        else if (line == "/all") {
            std::cout << "[전체 접속자 요청 전송]\n";
            SendOnlineList();

        }
        else if (line == "/ready" || line == "/unready") {
            const bool ready = (line == "/ready");
            if (CanRequest(Place::Room, false)) {
                if (g_game_phase != protobuf::GAME_WAITING && g_game_phase != protobuf::GAME_COUNTDOWN) {
                    std::cout << "[사용 불가] 게임 대기/카운트다운 중에만 준비할 수 있습니다.\n";
                }
                else {
                    SendGameReady(ready);
                }
            }

        }
        else if (line.rfind("/move", 0) == 0) {
            std::istringstream iss(line.substr(5));
            float x = 0.0f;
            float y = 0.0f;
            if (!(iss >> x >> y)) {
                std::cout << "사용법: /move <x> <y>  (예: /move 52 48)\n";
            }
            else if (CanRequest(Place::Room, false)) {
                if (g_game_phase != protobuf::GAME_PLAYING) {
                    std::cout << "[사용 불가] 게임 진행 중에만 이동할 수 있습니다.\n";
                }
                else {
                    SendGameMove(x, y, ++g_move_seq);
                }
            }

        }
        else if (line == "/logout") {
            if (CanRequest(Place::Any, true)) {
                std::cout << "[로그아웃 요청 전송]\n";
                SendPlaceRequest(SendLogout);
            }

        }
        else if (line[0] == '/') {
            std::cout << "알 수 없는 명령어. /help 를 입력하세요.\n";
        }
        else {
            // 슬래시로 시작하지 않으면 채팅. 길이는 UTF-8 변환 후 바이트로 확인(서버와 같은 기준)
            const std::string text = ToUtf8(line);
            if (text.size() > MAX_CHAT_BYTES) {
                std::cout << "[채팅 길이 초과] 한글 50자(" << MAX_CHAT_BYTES << "바이트)까지 보낼 수 있습니다. 현재 " << text.size() << "바이트\n";
            }
            else if (!g_chat_limit.TryTake()) {
                std::cout << "[채팅 제한] 너무 빠르게 보내고 있습니다(초당 2회, 연속 5회까지). 잠시 후 다시 입력하세요.\n";
            }
            else {
                SendChatMsg(text);
            }
        }
    }

    g_running = false;
    {
        // 수신 스레드의 블로킹 recv()를 깨움(소켓 닫기는 수신 스레드가 함)
        std::lock_guard<std::mutex> lock(g_send_mutex);
        if (g_sock != INVALID_SOCKET) shutdown(g_sock, SD_BOTH);
    }
    if (g_recv_thread.joinable()) g_recv_thread.join();
    { std::lock_guard<std::mutex> lock(g_heartbeat_mutex); }  // 대기 진입 직전 구간이 끝난 뒤 알림(깨우기 유실 방지)
    g_heartbeat_cv.notify_all();
    if (heartbeat_thread.joinable()) heartbeat_thread.join();

    WSACleanup();
    std::cout << "클라이언트 종료.\n";
    return 0;
}
