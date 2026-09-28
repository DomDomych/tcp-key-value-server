#include "metrics/metrics_server.hpp"

#include <boost/beast.hpp>
#include <memory>
#include <string>
#include <iostream>

namespace asio = boost::asio;
namespace beast = boost::beast;
namespace http = beast::http;

using tcp = asio::ip::tcp;

namespace
{
class MetricsSession : public std::enable_shared_from_this<MetricsSession>
{
  public:
    explicit MetricsSession(tcp::socket socket) : socket_(std::move(socket)) {};

    void start()
    {
        read();
    }

  private:
    void read()
    {
        auto self = shared_from_this();

        http::async_read(socket_, buffer_, request_,
                         [self](beast::error_code ec, std::size_t)
                         {
                             if (ec)
                                 return;

                             self->respond();
                         });
    }

    void respond()
    {
        response_.version(request_.version());
        response_.keep_alive(false);

        if(request_.method() == http::verb::get &&
           request_.target() == "/metrics" )
        {
            response_.result(http::status::ok);

            response_.set(
                http::field::content_type,
                "text/plain: version=0.0.4"
            );  

            response_.body() = "# metrics will be here\n";
        }

        else
        {
            response_.result(http::status::not_found);

            response_.set(http::field::content_type,"text/plain");

            response_.body() = "Not Found\n";
        }

        response_.prepare_payload();

        auto self = shared_from_this();

        http::async_write(
            socket_,
            response_,
            [self](beast::error_code,std::size_t)
            {
                beast::error_code ec;

                self->socket_.shutdown(
                    tcp::socket::shutdown_send,
                    ec
                );
            }
        );
    }

    tcp::socket socket_;

    boost::beast::flat_buffer buffer_;

    http::request<boost::beast::http::string_body> request_;

    http::response<boost::beast::http::string_body> response_;
};
} // namespace

MetricsServer::MetricsServer(
    asio::io_context& io_context,
    std::uint16_t port)
    : acceptor_(
        io_context,
        tcp::endpoint(tcp::v4(),port))
{
}

void MetricsServer::start()
{
    accept();
}

void MetricsServer::accept()
{
    acceptor_.async_accept(
        [this](beast::error_code ec,tcp::socket socket)
        {
            if(ec)
            {
                if(ec!=boost::asio::error::operation_aborted)
                {
                    std::cerr
                    << "Metrics accept error: "
                    << ec.message()
                    << '\n';
                }

                return;
            }

            std::make_shared<MetricsSession>(
                std::move(socket))->start();

            accept();
        }
    );
}

void MetricsServer::stop()
{
    boost::system::error_code ec;
    acceptor_.close(ec);
}