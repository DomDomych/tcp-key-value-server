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

} 

Server::Server(boost::asio::io_context &io, unsigned short port)
    : strand_(boost::asio::make_strand(io)),
      acceptor_(io, tcp::endpoint(tcp::v4(), port)),
      storage_(make_database_connection_string(), 100,4)
{
}

void Server::accept_client()
{
    acceptor_.async_accept(
        boost::asio::bind_executor(
            strand_,
            [this](boost::system::error_code ec, tcp::socket socket)
            {
                if (stopping_)
                {
                    return;
                }

                if (!ec)
                {
                    auto session = std::make_shared<Session>(
                        std::move(socket),
                        storage_,
                        [this](std::shared_ptr<Session> session)
                        {
                            boost::asio::post(
                                strand_,
                                [this, session = std::move(session)]()
                                {
                                    sessions_.erase(session);
                                });
                        });

                    sessions_.insert(session);
                    session->start();
                }

                if (!stopping_)
                {
                    accept_client();
                }
            }));
}

void Server::start()
{
    boost::asio::dispatch(
        strand_,
        [this]()
        {
            if (!stopping_)
            {
                accept_client();
            }
        });
}

void Server::stop()
{
    boost::asio::dispatch(
        strand_,
        [this]()
        {
            if (stopping_)
            {
                return;
            }

            stopping_ = true;

            boost::system::error_code ec;

            acceptor_.cancel(ec);
            acceptor_.close(ec);

            auto sessions = sessions_;

            for (auto &session : sessions)
            {
                session->stop();
            }
        });
}
