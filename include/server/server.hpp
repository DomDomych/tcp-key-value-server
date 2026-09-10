#pragma once

#include <boost/asio.hpp>
#include <string>
#include <unordered_map>
#include <mutex>


class Server
{
  public:
    Server(boost::asio::io_context &io, unsigned short port);

    void start();

  private:
    void accept_client();

    boost::asio::ip::tcp::acceptor acceptor_;

    std::unordered_map<std::string, std::string> storage_;
    std::mutex storage_mutex_;
};