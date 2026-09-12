#pragma once

#include <boost/asio.hpp>
#include <string>
#include <unordered_map>
#include <storage.hpp>

using tcp = boost::asio::ip::tcp;

class Session
{
  public:
    explicit Session(tcp::socket socket,
                     Storage &server_storage);

    void start();

  private:
    void read();
    bool write(const std::string &message);

    tcp::socket socket_;
    std::string buffer_;

    Storage& server_storage_;
};