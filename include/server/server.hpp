#pragma once

#include <boost/asio.hpp>
#include <mutex>
#include <storage/storage.hpp>
#include <string>

class Server
{
  public:
    Server(boost::asio::io_context &io, unsigned short port);

    void start();

  private:
    void accept_client();

    boost::asio::ip::tcp::acceptor acceptor_;

    Storage storage_;
};