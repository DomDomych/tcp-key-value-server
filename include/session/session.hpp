#pragma once

#include <boost/asio.hpp>
#include <storage/storage.hpp>
#include <string>
#include <memory>

using tcp = boost::asio::ip::tcp;

class Session : public std::enable_shared_from_this<Session>
{
  public:
    explicit Session(tcp::socket socket, Storage &server_storage);

    void start();

  private:
    void read();
    bool write(const std::string &message);

    tcp::socket socket_;
    std::string buffer_;
    std::string response_;

    Storage &server_storage_;
};