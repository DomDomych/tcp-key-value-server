#include "session/session.hpp"

#include "protocol/command_processor.hpp"
#include "protocol/parser.hpp"
#include "protocol/request.hpp"

#include <boost/asio.hpp>
#include <string_view>
#include <utility>

using tcp = boost::asio::ip::tcp;

Session::Session(tcp::socket socket, Storage &server_storage, CloseHandler on_close)
    : socket_(std::move(socket)), strand_(boost::asio::make_strand(socket_.get_executor())),
      server_storage_(server_storage), on_close_(std::move(on_close))
{
}

void Session::start()
{
    auto self = shared_from_this();

    boost::asio::dispatch(strand_,
                          [self]()
                          {
                              if (!self->stopping_ && !self->closed_)
                              {
                                  self->read();
                              }
                          });
}

void Session::read()
{
    auto self = shared_from_this();

    boost::asio::async_read_until(
        socket_, boost::asio::dynamic_buffer(buffer_), '\n',
        boost::asio::bind_executor(strand_,
                                   [self](boost::system::error_code ec, std::size_t bytes)
                                   {
                                       if (ec)
                                       {
                                           self->close();
                                           return;
                                       }

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
                                   }));
}

void Session::write()
{
    writing_ = true;

    auto self = shared_from_this();

    boost::asio::async_write(
        socket_, boost::asio::buffer(response_),
        boost::asio::bind_executor(strand_,
                                   [self](boost::system::error_code ec, std::size_t)
                                   {
                                       self->writing_ = false;

                                       if (ec)
                                       {
                                           self->close();
                                           return;
                                       }

                                       if (self->stopping_)
                                       {
                                           self->close();
                                           return;
                                       }

                                       self->read();
                                   }));
}

void Session::stop()
{
    auto self = shared_from_this();

    boost::asio::dispatch(strand_,
                          [self]()
                          {
                              if (self->closed_ || self->stopping_)
                              {
                                  return;
                              }

                              self->stopping_ = true;

                              // Если сейчас отправляем результат уже обработанной команды,
                              // позволяем async_write закончиться.
                              if (self->writing_)
                              {
                                  return;
                              }

                              // Иначе Session просто ждёт следующую команду —
                              // её можно закрывать сразу.
                              self->close();
                          });
}

void Session::close()
{
    if (closed_)
    {
        return;
    }

    closed_ = true;

    boost::system::error_code ec;

    socket_.shutdown(tcp::socket::shutdown_both, ec);
    socket_.close(ec);

    if (on_close_)
    {
        on_close_(shared_from_this());
    }
}