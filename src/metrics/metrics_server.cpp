#include "metrics/metrics_server.hpp"

#include <boost/beast.hpp>
#include <memory>
#include <string>

namespace asio = boost::asio;
namespace beast = boost::beast;
namespace htpp = beast::http;

using tcp = asio::ip::tcp;

namespace
{
class MetricsSession : public std::enable_shared_from_this<MetricsSession>
{
  public:
    explicit MetricsSession(tcp::socket socket) : socket_(std::move(socket)) {};

    void start
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
                         })
    }

    void respond()
    {
        response_.version(request_.version());
        response_.keep_alive(false);

        if(request_.method() == )
    }

    tcp::socket socket_;

    boost::beast::flat_buffer buffer_;

    http::request<boost::beast::http::string_body> request_;

    http::response<boost::beast::http::string_body> response_;
};
} // namespace