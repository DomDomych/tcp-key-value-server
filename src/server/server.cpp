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
    tcp::socket socket = acceptor_.accept();

    std::thread client_thread(
        [this, socket = std::move(socket)]() mutable
        {
            Session session(std::move(socket), storage_, storage_mutex_);
            session.start();
        });

    client_thread.detach();
}

void Server::start()
{

    for (;;)
    {
        accept_client();
    }
}
