#pragma once

#include <boost/asio.hpp>
#include <functional>
#include <memory>
#include <storage/storage.hpp>
#include <string>

using tcp = boost::asio::ip::tcp;

class Session : public std::enable_shared_from_this<Session>
{
  public:
    using CloseHandler = std::function<void(std::shared_ptr<Session>)>;

    Session(tcp::socket socket, Storage &server_storage, CloseHandler on_close);

    void start();
    void stop();

  private:
    void read();
    void write();
    void close();

    tcp::socket socket_;
    boost::asio::strand<tcp::socket::executor_type> strand_;

    std::string buffer_;
    std::string response_;

    Storage &server_storage_;
    CloseHandler on_close_;

    bool stopping_{false};
    bool writing_{false};
    bool closed_{false};
};