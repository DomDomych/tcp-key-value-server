#include "server/server.hpp"
#include "session/session.hpp"
#include <boost/asio.hpp>
#include <thread>

using tcp = boost::asio::ip::tcp;

Server::Server(boost::asio::io_context &io, unsigned short port)
    : acceptor_(io, tcp::endpoint(tcp::v4(), port))
{
}

void Server::accept_client()
{
    acceptor_.async_accept(
        [this](boost::system::error_code ec,tcp::socket socket)
        {
            if(!ec)
            {
                std::make_shared<Session>(
                    std::move(socket),
                    storage_
                )->start();
            }
            accept_client();
        }
    );
}

void Server::start()
{

    for (;;)
    {
        accept_client();
    }
}
