#define _CRT_SECURE_NO_WARNINGS

#include <boost/asio.hpp>
#include <boost/bind/bind.hpp>
#include <boost/thread.hpp>
#include <iostream>
#include <string>
#include <deque>
#include <cstring>
#include <sstream>

using boost::asio::ip::tcp;

// 메시지 타입 정의 (서버와 동일)
enum MessageType {
    MSG_JOIN_ROOM = 1,
    MSG_LEAVE_ROOM = 2,
    MSG_CHAT = 3,
    MSG_ROOM_LIST = 4,
    MSG_USER_LIST = 5
};

// 메시지 구조체 (서버와 동일)
struct Message {
    uint32_t type;
    uint32_t room_id;
    uint32_t user_id;
    uint32_t data_length;
    char data[512];

    Message() : type(0), room_id(0), user_id(0), data_length(0) {
        memset(data, 0, sizeof(data));
    }
};

// 데이터 구조체
struct CardData {
    uint32_t user_id;
    char user_name[100];

    CardData() : user_id(0) {
        memset(user_name, 0, sizeof(user_name));
    }
};

// 클라이언트 클래스
class RoomClient {
public:
    RoomClient(boost::asio::io_context& io_context)
        : _io_context(io_context), _socket(io_context),
        _strand(boost::asio::make_strand(io_context)),
        _current_room_id(0), _user_id(0) {
    }

    void connect(const std::string& host, const std::string& port) {
        tcp::resolver resolver(_io_context);
        auto endpoints = resolver.resolve(host, port);

        boost::asio::async_connect(_socket, endpoints,
            boost::bind(&RoomClient::handle_connect, this,
                boost::asio::placeholders::error));
    }

    void disconnect() {
        boost::asio::post(_strand,
            boost::bind(&RoomClient::do_close, this));
    }

    // 룸 입장
    void join_room(uint32_t room_id) {
        Message msg;
        msg.type = MSG_JOIN_ROOM;
        msg.room_id = room_id;
        msg.user_id = _user_id;
        msg.data_length = 0;
        send_message(msg);
    }

    // 룸 퇴장
    void leave_room() {
        if (_current_room_id > 0) {
            Message msg;
            msg.type = MSG_LEAVE_ROOM;
            msg.room_id = _current_room_id;
            msg.user_id = _user_id;
            msg.data_length = 0;
            send_message(msg);
        }
    }

    // 채팅 메시지 전송
    void send_chat(const std::string& text) {
        if (_current_room_id > 0) {
            Message msg;
            msg.type = MSG_CHAT;
            msg.room_id = _current_room_id;
            msg.user_id = _user_id;
            msg.data_length = text.length();
            strcpy(msg.data, text.c_str());
            send_message(msg);
        }
        else {
            std::cout << "You need to join a room first!" << std::endl;
        }
    }

    // 룸 목록 요청
    void request_room_list() {
        Message msg;
        msg.type = MSG_ROOM_LIST;
        msg.user_id = _user_id;
        msg.data_length = 0;
        send_message(msg);
    }

    // 사용자 목록 요청
    void request_user_list() {
        if (_current_room_id > 0) {
            Message msg;
            msg.type = MSG_USER_LIST;
            msg.room_id = _current_room_id;
            msg.user_id = _user_id;
            msg.data_length = 0;
            send_message(msg);
        }
        else {
            std::cout << "You need to join a room first!" << std::endl;
        }
    }

    bool is_connected() const {
        return _socket.is_open();
    }

    uint32_t get_current_room_id() const {
        return _current_room_id;
    }

private:
    void handle_connect(const boost::system::error_code& error) {
        if (!error) {
            std::cout << "Connected to server!" << std::endl;
            start_read();
        }
        else {
            std::cout << "Connect failed: " << error.message() << std::endl;
        }
    }

    void start_read() {
        boost::asio::async_read(_socket,
            boost::asio::buffer(&_read_msg, sizeof(Message)),
            boost::asio::bind_executor(_strand,
                boost::bind(&RoomClient::handle_read, this,
                    boost::asio::placeholders::error)));
    }

    void handle_read(const boost::system::error_code& error) {
        if (!error) {
            process_message(_read_msg);
            start_read();
        }
        else {
            std::cout << "Read error: " << error.message() << std::endl;
            do_close();
        }
    }

    void process_message(const Message& msg) {
        switch (msg.type) {
        case MSG_JOIN_ROOM:
            handle_join_room_response(msg);
            break;
        case MSG_LEAVE_ROOM:
            handle_leave_room_response(msg);
            break;
        case MSG_CHAT:
            handle_chat_message(msg);
            break;
        case MSG_ROOM_LIST:
            handle_room_list_response(msg);
            break;
        case MSG_USER_LIST:
            handle_user_list_response(msg);
            break;
        default:
            std::cout << "Unknown message type received: " << msg.type << std::endl;
        }
    }

    void handle_join_room_response(const Message& msg) {
        _current_room_id = msg.room_id;
        std::cout << "\n=== " << msg.data << " ===" << std::endl;
        std::cout << "Current room: " << _current_room_id << std::endl;
    }

    void handle_leave_room_response(const Message& msg) {
        _current_room_id = 0;
        std::cout << "\n=== " << msg.data << " ===" << std::endl;
    }

    void handle_chat_message(const Message& msg) {
        if (msg.user_id == 0) {
            // 시스템 메시지
            std::cout << "[SYSTEM] " << msg.data << std::endl;
        }
        else {
            // 일반 채팅 메시지
            std::cout << "[User " << msg.user_id << "] " << msg.data << std::endl;
        }
    }

    void handle_room_list_response(const Message& msg) {
        std::cout << "\n=== Room List ===" << std::endl;
        std::cout << msg.data << std::endl;
    }

    void handle_user_list_response(const Message& msg) {
        std::cout << "\n=== User List ===" << std::endl;
        std::cout << msg.data << std::endl;
    }

    void send_message(const Message& msg) {
        boost::asio::post(_strand,
            boost::bind(&RoomClient::do_send_message, this, msg));
    }

    void do_send_message(const Message& msg) {
        bool write_in_progress = !_write_msgs.empty();
        _write_msgs.push_back(msg);
        if (!write_in_progress) {
            start_write();
        }
    }

    void start_write() {
        boost::asio::async_write(_socket,
            boost::asio::buffer(&_write_msgs.front(), sizeof(Message)),
            boost::asio::bind_executor(_strand,
                boost::bind(&RoomClient::handle_write, this,
                    boost::asio::placeholders::error)));
    }

    void handle_write(const boost::system::error_code& error) {
        if (!error) {
            _write_msgs.pop_front();
            if (!_write_msgs.empty()) {
                start_write();
            }
        }
        else {
            std::cout << "Write error: " << error.message() << std::endl;
            do_close();
        }
    }

    void do_close() {
        _socket.close();
        std::cout << "Disconnected from server." << std::endl;
    }

    boost::asio::io_context& _io_context;
    tcp::socket _socket;
    boost::asio::strand<boost::asio::io_context::executor_type> _strand;
    Message _read_msg;
    std::deque<Message> _write_msgs;
    uint32_t _current_room_id;
    uint32_t _user_id;
};

// 사용자 인터페이스 클래스
class UserInterface {
public:
    UserInterface(RoomClient& client) : _client(client) {}

    void show_menu() {
        std::cout << "\n=== Room Chat Client ===" << std::endl;
        std::cout << "Commands:" << std::endl;
        std::cout << "  /join <room_id>  - Join a room" << std::endl;
        std::cout << "  /leave           - Leave current room" << std::endl;
        std::cout << "  /rooms           - Show room list" << std::endl;
        std::cout << "  /users           - Show users in current room" << std::endl;
        /*std::cout << "  /help            - Show this help" << std::endl;*/
        std::cout << "  /quit            - Exit program" << std::endl;
        std::cout << "  <message>        - Send chat message" << std::endl;
        std::cout << "======================" << std::endl;
    }

    void run() {
        show_menu();

        std::string input;
        while (_client.is_connected() && std::getline(std::cin, input)) {
            if (input.empty()) continue;

            if (input[0] == '/') {
                process_command(input);
            }
            else {
                // 일반 채팅 메시지
                _client.send_chat(input);
            }

            if (input == "/quit") {
                break;
            }
        }
    }

private:
    void process_command(const std::string& input) {
        std::istringstream iss(input);
        std::string command;
        iss >> command;

        if (command == "/join") {
            uint32_t room_id;
            if (iss >> room_id) {
                _client.join_room(room_id);
            }
            else {
                std::cout << "Usage: /join <room_id>" << std::endl;
            }
        }
        else if (command == "/leave") {
            _client.leave_room();
        }
        else if (command == "/rooms") {
            _client.request_room_list();
        }
        else if (command == "/users") {
            _client.request_user_list();
        }
        /*else if (command == "/help") {
            show_menu();
        }*/
        else if (command == "/quit") {
            std::cout << "Disconnecting..." << std::endl;
            _client.disconnect();
        }
        else {
            std::cout << "Unknown command: " << command << std::endl;
            std::cout << "Type /help for available commands." << std::endl;
        }
    }

    RoomClient& _client;
};

// 메인 함수
int main(int argc, char* argv[]) {
    std::string host = "127.0.0.1";
    std::string port = "12345";

    // 명령행 인수로 호스트와 포트 지정 가능
    if (argc == 3) {
        host = argv[1];
        port = argv[2];
    }

    try {
        boost::asio::io_context io_context;
        RoomClient client(io_context);

        std::cout << "Connecting to " << host << ":" << port << "..." << std::endl;
        client.connect(host, port);

        // IO 컨텍스트를 별도 스레드에서 실행
        boost::thread io_thread(boost::bind(&boost::asio::io_context::run, &io_context));

        // 연결 대기
        boost::this_thread::sleep(boost::posix_time::milliseconds(1000));

        if (client.is_connected()) {
            // 사용자 인터페이스 실행
            UserInterface ui(client);
            ui.run();
        }
        else {
            std::cout << "Failed to connect to server." << std::endl;
        }

        // 정리
        client.disconnect();
        io_context.stop();
        io_thread.join();

    }
    catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }

    return 0;
}