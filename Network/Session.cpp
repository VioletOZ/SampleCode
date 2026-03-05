#include "stdafx.h"
#include "Session.h"

void Session::Start()
{   
    _session_uuid = boost::uuids::to_string(boost::uuids::random_generator()());

    std::cout << "Connect Session UUID : " << _session_uuid << std::endl;

    ReadHeader();
}

void Session::SendMsg(const Message& message)
{
    std::string out;
    message.SerializeToString(&out);

    boost::asio::post(_strand, boost::bind(&Session::DoSendMsg, shared_from_this(), out));
}

void Session::ReadHeader()
{
    boost::asio::async_read(_socket,
        boost::asio::buffer(&_read_msg, sizeof(Message)),
        boost::asio::bind_executor(_strand,
            boost::bind(&Session::HandleReadHeader, shared_from_this(),
                boost::asio::placeholders::error)));
}

void Session::HandleReadHeader(const boost::system::error_code& error)
{
    if (!error) {
        HandleMessage();
        ReadHeader();
    }
    else {
        HandleDisconnect();
    }
}

void Session::HandleMessage()
{
    _room_manager.HandleMessage(shared_from_this(), _read_msg);
}

void Session::DoSendMsg(const std::string& serialized)
{
    bool write_in_progress = !_write_msgs.empty();
    _write_msgs.push_back(serialized);
    if (!write_in_progress) {
        Write();
    }
}

void Session::Write()
{
    auto msg = _write_msgs.front();

    boost::asio::async_write(_socket,
        boost::asio::buffer(&_write_msgs.front(), sizeof(msg)),
        boost::asio::bind_executor(_strand,
            boost::bind(&Session::HandleWrite, shared_from_this(),
                boost::asio::placeholders::error)));
}

void Session::HandleWrite(const boost::system::error_code& error)
{
    if (!error) {
        _write_msgs.pop_front();
        if (!_write_msgs.empty()) {
            Write();
        }
    }
    else {
        HandleDisconnect();
    }
}

void Session::HandleDisconnect()
{
    std::cout << "User " << GetUserId() << " disconnected" << std::endl;
    if (_current_room_id > 0) {
        Session::LeaveCurrentRoom();
    }
    _socket.close();
}

void Session::LeaveCurrentRoom()
{
    if (_current_room_id > 0) {
        auto room = _room_manager.GetRoom(_current_room_id);
        if (room) {
            room->Leave(shared_from_this());
        }
    }
}