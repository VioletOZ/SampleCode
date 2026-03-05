#pragma once
#include "stdafx.h"
#include "Session.h"
#include "RoomManager.h"

class Server {
public:
    Server(boost::asio::io_context& io_context, short port);

private:
    void StartAccept();
    void HandleAccept(SessionPtr session, const boost::system::error_code& error);

    boost::asio::io_context& _io_context;
    boost::asio::ip::tcp::tcp::acceptor _acceptor;
    RoomManager _room_manager;

    // Session UUID, Session
    std::unordered_map<std::string, SessionPtr> _session_map;
};