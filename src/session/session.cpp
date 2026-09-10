#include "session/session.hpp"
#include "protocol/command_processor.hpp"
#include "protocol/parser.hpp"
#include "protocol/request.hpp"

#include <boost/asio.hpp>
#include <string_view>

using tcp = boost::asio::ip::tcp;

Session::Session(tcp::socket socket, std::unordered_map<std::string, std::string> &server_storage,
                 std::mutex &storage_mutex)
    : socket_(std::move(socket)), server_storage_(server_storage), storage_mutex_(storage_mutex)
{
}

void Session::read()
{
    for (;;)
    {
        boost::system::error_code ec;

        std::size_t bytes =
            boost::asio::read_until(socket_, boost::asio::dynamic_buffer(buffer_), '\n', ec);

        if (ec)
            break;
        std::string_view temp_data{buffer_.data(), bytes};

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

        std::string response;

        {
            std::lock_guard<std::mutex> lock(storage_mutex_);
            response = process(req, server_storage_);
        }

        if (!write(response))
        {
            break;
        }

        buffer_.erase(0, bytes);
    }
}

bool Session::write(const std::string &message)
{
    boost::system::error_code ec;

    boost::asio::write(socket_, boost::asio::buffer(message), ec);

    return !ec;
}

void Session::start()
{
    read();
}
