#include "session/session.hpp"
#include "protocol/command_processor.hpp"
#include "protocol/parser.hpp"
#include "protocol/request.hpp"

#include <boost/asio.hpp>
#include <string_view>

using tcp = boost::asio::ip::tcp;

Session::Session(tcp::socket socket, Storage &server_storage)
    : socket_(std::move(socket)), server_storage_(server_storage)
{
}

void Session::read()
{
    auto self = shared_from_this();
    boost::asio::async_read_until(socket_, boost::asio::dynamic_buffer(buffer_), '\n',
                                  [self](boost::system::error_code ec, std::size_t bytes)
                                  {
                                      if (ec)
                                          return;

                                      std::string_view temp_data{self->buffer_.data(), bytes};

                                      if (!temp_data.empty() && temp_data.back() == '\n')
                                      {
                                          temp_data.remove_suffix(1);
                                      }

                                      if (!temp_data.empty() && temp_data.back() == '\r')
                                      {
                                          temp_data.remove_suffix(1);
                                      }

                                      Request req{};

                                      parse(req, temp_data);

                                      self->response_ = process(req, self->server_storage_);

                                      self->buffer_.erase(0, bytes);

                                      self->write();
                                  });
}

void Session::write()
{
    auto self = shared_from_this();

    boost::asio::async_write(socket_, boost::asio::buffer(response_),
                             [self](boost::system::error_code ec, std::size_t)
                             {
                                 if (ec)
                                     return;

                                 self->read();
                             });
}

void Session::start()
{
    read();
}
