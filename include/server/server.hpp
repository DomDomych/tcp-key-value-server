#pragma once

#include <boost/asio.hpp>
#include <mutex>
#include <string>
#include <unordered_map>
#include <storage/storage.hpp>

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