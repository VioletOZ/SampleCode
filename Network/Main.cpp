#include "stdafx.h"
#include "Server.h"


////////////////////////////////////////////////////////////////////////
// 1. Server io_context 및 port initialization
// 2. Server::StartAccept()를 호출하여 클라이언트 연결을 기다립니다.
// 3. 클라이언트가 연결되면, Server::HandleAccept()가 호출되어 새로운 세션을 생성합니다.
// 4. 클라이언트 세션은 비동기적으로 메시지를 읽고 처리합니다.
// 5. 클라이언트가 메시지를 보내면, Server::HandleRead()가 호출되어 메시지를 처리합니다.
////////////////////////////////////////////////////////////////////////
int main(int argc, char* argv[]) 
{
    
    try {
        boost::asio::io_context io_context;

        Server server(io_context, 12345);
        std::cout << "Room Server started on port 12345" << std::endl;

        io_context.run();
    }
    catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }

    return 0;
}