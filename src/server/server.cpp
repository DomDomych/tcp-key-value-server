#include "server/server.hpp"
#include "session/session.hpp"
#include <boost/asio.hpp>
#include <cstdlib>
#include <string>
#include <thread>

using tcp = boost::asio::ip::tcp;

namespace
{

std::string get_env(const char *name, const char *default_value)
{
    const char *value = std::getenv(name);

    if (value != nullptr)
    {
        return value;
    }

    return default_value;
}

std::string make_database_connection_string()
{
    return "host=" + get_env("DB_HOST", "localhost") +
           " "
           "port=" +
           get_env("DB_PORT", "5432") +
           " "
           "dbname=" +
           get_env("DB_NAME", "kv_server") +
           " "
           "user=" +
           get_env("DB_USER", "kv_user") +
           " "
           "password=" +
           get_env("DB_PASSWORD", "1234");
}

} // namespace

Server::Server(boost::asio::io_context &io, unsigned short port)
    : acceptor_(io, tcp::endpoint(tcp::v4(), port)),
      storage_(make_database_connection_string(), 100,4)
{
}

void Server::accept_client()
{
    acceptor_.async_accept(
        [this](boost::system::error_code ec, tcp::socket socket)
        {
            if (!ec)
            {
                auto session = std::make_shared<Session>(std::move(socket), storage_);

                session->start();
            }
            accept_client();
        });
}

void Server::start()
{

    accept_client();
}
