#pragma once

#include <boost/asio.hpp>
#include <memory>
#include <storage/storage.hpp>
#include <unordered_set>

class Session;

class Server
{
  public:
    Server(boost::asio::io_context &io, unsigned short port);

    void start();
    void stop();

  private:
    void accept_client();

    boost::asio::strand<boost::asio::io_context::executor_type> strand_;

    boost::asio::ip::tcp::acceptor acceptor_;

    Storage storage_;

    std::unordered_set<std::shared_ptr<Session>> sessions_;

    bool stopping_{false};
};