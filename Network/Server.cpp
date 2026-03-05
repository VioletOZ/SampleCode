#include "stdafx.h"
#include "Server.h"

Server::Server(boost::asio::io_context& io_context, short port) : _io_context(io_context), _acceptor(io_context, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port))
{
    StartAccept();
}

void Server::StartAccept()
{
    SessionPtr new_session = boost::make_shared<Session>(_io_context, _room_manager);
    _acceptor.async_accept(new_session->socket(),
        boost::bind(&Server::HandleAccept, this, new_session, boost::asio::placeholders::error));
}

void Server::HandleAccept(SessionPtr session, const boost::system::error_code& error)
{
    if (!error) {
        session->Start();

        _session_map.try_emplace(session->GetSessionId(), session);        
    }

    StartAccept();
}