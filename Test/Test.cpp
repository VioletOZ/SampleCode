#define _CRT_SECURE_NO_WARNINGS

#include <boost/asio.hpp>
#include <boost/bind/bind.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <memory>
#include <functional>
#include <cstring>
#include <deque>



using boost::asio::ip::tcp;

// 메시지 타입 정의
enum MessageType {
    MSG_JOIN_ROOM = 1,
    MSG_LEAVE_ROOM = 2,
    MSG_CHAT = 3,
    MSG_ROOM_LIST = 4,
    MSG_USER_LIST = 5
};

// 메시지 구조체
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

// 전방 선언
class Session;
class Room;
class RoomManager;

typedef boost::shared_ptr<Session> SessionPtr;
typedef boost::shared_ptr<Room> RoomPtr;

// 세션 클래스
class Session : public boost::enable_shared_from_this<Session> {
public:
    Session(boost::asio::io_context& io_context, RoomManager& room_manager)
        : _socket(io_context), _room_manager(room_manager),
        _strand(boost::asio::make_strand(io_context)), _user_id(0), _current_room_id(0) {
    }

    tcp::socket& socket() { return _socket; }

    void start() {
        _user_id = generate_user_id();
        std::cout << "Session started for user: " << _user_id << std::endl;
        read_header();
    }

    void send_message(const Message& msg) {
        boost::asio::post(_strand,
            boost::bind(&Session::do_send_message, shared_from_this(), msg));
    }

    uint32_t get_user_id() const { return _user_id; }
    uint32_t get_current_room_id() const { return _current_room_id; }
    void set_current_room_id(uint32_t room_id) { _current_room_id = room_id; }

private:
    void read_header() {
        boost::asio::async_read(_socket,
            boost::asio::buffer(&_read_msg, sizeof(Message)),
            boost::asio::bind_executor(_strand,
                boost::bind(&Session::handle_read_header, shared_from_this(),
                    boost::asio::placeholders::error)));
    }

    void handle_read_header(const boost::system::error_code& error) {
        if (!error) {
            handle_message();
            read_header();
        }
        else {
            handle_disconnect();
        }
    }

    void handle_message();

    void do_send_message(const Message& msg) {
        bool write_in_progress = !_write_msgs.empty();
        _write_msgs.push_back(msg);
        if (!write_in_progress) {
            write();
        }
    }

    void write() {
        boost::asio::async_write(_socket,
            boost::asio::buffer(&_write_msgs.front(), sizeof(Message)),
            boost::asio::bind_executor(_strand,
                boost::bind(&Session::handle_write, shared_from_this(),
                    boost::asio::placeholders::error)));
    }

    void handle_write(const boost::system::error_code& error) {
        if (!error) {
            _write_msgs.pop_front();
            if (!_write_msgs.empty()) {
                write();
            }
        }
        else {
            handle_disconnect();
        }
    }

    void handle_disconnect() {
        std::cout << "User " << _user_id << " disconnected" << std::endl;
        if (_current_room_id > 0) {
            leave_current_room();
        }
        _socket.close();
    }

    void leave_current_room();

    uint32_t generate_user_id() {
        static uint32_t id_counter = 1000;
        return ++id_counter;
    }

    tcp::socket _socket;
    RoomManager& _room_manager;
    boost::asio::strand<boost::asio::io_context::executor_type> _strand;
    Message _read_msg;
    std::deque<Message> _write_msgs;
    uint32_t _user_id;
    uint32_t _current_room_id;
};


// 룸 클래스
class Room {
public:
    Room(uint32_t id, const std::string& name) : _id(id), _name(name) {}

    void join(SessionPtr session) {
        _members.insert(session);
        session->set_current_room_id(_id);

        // 입장 알림 메시지 브로드캐스트
        Message msg;
        msg.type = MSG_CHAT;
        msg.room_id = _id;
        msg.user_id = 0; // 시스템 메시지
        std::string join_msg = "User " + std::to_string(session->get_user_id()) + " joined the room";
        msg.data_length = join_msg.length();
        strcpy(msg.data, join_msg.c_str());

        broadcast(msg);
        std::cout << "User " << session->get_user_id() << " joined room " << _id << std::endl;
    }

    void leave(SessionPtr session) {
        _members.erase(session);
        session->set_current_room_id(0);

        // 퇴장 알림 메시지 브로드캐스트
        Message msg;
        msg.type = MSG_CHAT;
        msg.room_id = _id;
        msg.user_id = 0; // 시스템 메시지
        std::string leave_msg = "User " + std::to_string(session->get_user_id()) + " left the room";
        msg.data_length = leave_msg.length();
        strcpy(msg.data, leave_msg.c_str());

        broadcast(msg);
        std::cout << "User " << session->get_user_id() << " left room " << _id << std::endl;
    }

    void broadcast(const Message& msg) {
        for (auto session : _members) {
            session->send_message(msg);
        }
    }

    uint32_t get_id() const { return _id; }
    const std::string& get_name() const { return _name; }
    size_t get_member_count() const { return _members.size(); }

    std::vector<uint32_t> get_member_ids() const {
        std::vector<uint32_t> ids;
        for (auto session : _members) {
            ids.push_back(session->get_user_id());
        }
        return ids;
    }

private:
    uint32_t _id;
    std::string _name;
    std::set<SessionPtr> _members;
};

// 메시지 핸들러 클래스
class MessageHandler {
public:
    MessageHandler(RoomManager& room_manager) : _room_manager(room_manager) {}

    void handle_message(SessionPtr session, const Message& msg) {
        switch (msg.type) {
        case MSG_JOIN_ROOM:
            handle_join_room(session, msg);
            break;
        case MSG_LEAVE_ROOM:
            handle_leave_room(session, msg);
            break;
        case MSG_CHAT:
            handle_chat_message(session, msg);
            break;
        case MSG_ROOM_LIST:
            handle_room_list_request(session, msg);
            break;
        case MSG_USER_LIST:
            handle_user_list_request(session, msg);
            break;
        default:
            std::cout << "Unknown message type: " << msg.type << std::endl;
        }
    }

private:
    void handle_join_room(SessionPtr session, const Message& msg);
    void handle_leave_room(SessionPtr session, const Message& msg);
    void handle_chat_message(SessionPtr session, const Message& msg);
    void handle_room_list_request(SessionPtr session, const Message& msg);
    void handle_user_list_request(SessionPtr session, const Message& msg);

    RoomManager& _room_manager;
};

// 룸 매니저 클래스
class RoomManager {
public:
    RoomManager() : _message_handler(*this) {
        // 기본 룸들 생성
        create_room(1, "General");
        create_room(2, "Gaming");
        create_room(3, "Development");
    }

    RoomPtr get_room(uint32_t room_id) {
        auto it = _rooms.find(room_id);
        return (it != _rooms.end()) ? it->second : RoomPtr();
    }

    RoomPtr create_room(uint32_t room_id, const std::string& name) {
        auto room = boost::make_shared<Room>(room_id, name);
        _rooms[room_id] = room;
        std::cout << "Room created: " << room_id << " - " << name << std::endl;
        return room;
    }

    void handle_message(SessionPtr session, const Message& msg) {
        _message_handler.handle_message(session, msg);
    }

    std::map<uint32_t, RoomPtr> get_all_rooms() const {
        return _rooms;
    }

private:
    std::map<uint32_t, RoomPtr> _rooms;
    MessageHandler _message_handler;
};

// Session 클래스의 메서드 구현
void Session::handle_message() {
    _room_manager.handle_message(shared_from_this(), _read_msg);
}

void Session::leave_current_room() {
    if (_current_room_id > 0) {
        auto room = _room_manager.get_room(_current_room_id);
        if (room) {
            room->leave(shared_from_this());
        }
    }
}

// MessageHandler 메서드 구현
void MessageHandler::handle_join_room(SessionPtr session, const Message& msg) {
    auto room = _room_manager.get_room(msg.room_id);
    if (room) {
        // 현재 룸에서 나가기
        if (session->get_current_room_id() > 0) {
            auto current_room = _room_manager.get_room(session->get_current_room_id());
            if (current_room) {
                current_room->leave(session);
            }
        }

        // 새 룸에 입장
        room->join(session);

        // 입장 확인 메시지 전송
        Message response;
        response.type = MSG_JOIN_ROOM;
        response.room_id = msg.room_id;
        response.user_id = session->get_user_id();
        std::string success_msg = "Successfully joined room " + room->get_name();
        response.data_length = success_msg.length();
        strcpy(response.data, success_msg.c_str());
        session->send_message(response);
    }
}

void MessageHandler::handle_leave_room(SessionPtr session, const Message& msg) {
    auto room = _room_manager.get_room(session->get_current_room_id());
    if (room) {
        room->leave(session);

        Message response;
        response.type = MSG_LEAVE_ROOM;
        response.room_id = msg.room_id;
        response.user_id = session->get_user_id();
        std::string leave_msg = "Left room " + room->get_name();
        response.data_length = leave_msg.length();
        strcpy(response.data, leave_msg.c_str());
        session->send_message(response);
    }
}

void MessageHandler::handle_chat_message(SessionPtr session, const Message& msg) {
    auto room = _room_manager.get_room(session->get_current_room_id());
    if (room) {
        Message broadcast_msg = msg;
        broadcast_msg.user_id = session->get_user_id();
        room->broadcast(broadcast_msg);
        std::cout << "Chat in room " << room->get_id() << " from user "
            << session->get_user_id() << ": " << msg.data << std::endl;
    }
}

void MessageHandler::handle_room_list_request(SessionPtr session, const Message& msg) {
    Message response;
    response.type = MSG_ROOM_LIST;
    response.user_id = session->get_user_id();

    std::string room_list = "Available rooms:\n";
    auto rooms = _room_manager.get_all_rooms();
    for (const auto& pair : rooms) {
        auto room = pair.second;
        room_list += std::to_string(room->get_id()) + ": " + room->get_name()
            + " (" + std::to_string(room->get_member_count()) + " users)\n";
    }

    response.data_length = room_list.length();
    strcpy(response.data, room_list.c_str());
    session->send_message(response);
}

void MessageHandler::handle_user_list_request(SessionPtr session, const Message& msg) {
    auto room = _room_manager.get_room(session->get_current_room_id());
    if (room) {
        Message response;
        response.type = MSG_USER_LIST;
        response.room_id = room->get_id();
        response.user_id = session->get_user_id();

        std::string user_list = "Users in room " + room->get_name() + ":\n";
        auto member_ids = room->get_member_ids();
        for (uint32_t id : member_ids) {
            user_list += "User " + std::to_string(id) + "\n";
        }

        response.data_length = user_list.length();
        strcpy(response.data, user_list.c_str());
        session->send_message(response);
    }
}

// 서버 클래스
class RoomServer {
public:
    RoomServer(boost::asio::io_context& io_context, short port)
        : _io_context(io_context), _acceptor(io_context, tcp::endpoint(tcp::v4(), port)) {
        start_accept();
    }

private:
    void start_accept() {
        SessionPtr new_session = boost::make_shared<Session>(_io_context, _room_manager);
        _acceptor.async_accept(new_session->socket(),
            boost::bind(&RoomServer::handle_accept, this, new_session,
                boost::asio::placeholders::error));
    }

    void handle_accept(SessionPtr session, const boost::system::error_code& error) {
        if (!error) {
            session->start();
        }
        start_accept();
    }

    boost::asio::io_context& _io_context;
    tcp::acceptor _acceptor;
    RoomManager _room_manager;
};

// 메인 함수
int main() {
    try {
        boost::asio::io_context io_context;

        RoomServer server(io_context, 12345);
        std::cout << "Room Server started on port 12345" << std::endl;

        io_context.run();
    }
    catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }

    return 0;
}