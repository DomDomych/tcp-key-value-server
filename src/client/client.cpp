#include <boost/asio.hpp>

#include <cstdlib>
#include <iostream>
#include <string>

using tcp = boost::asio::ip::tcp;

int main()
{
    const char* host_env = std::getenv("SERVER_HOST");
    const char* port_env = std::getenv("SERVER_PORT");

    const std::string host =
        host_env != nullptr ? host_env : "127.0.0.1";

    const std::string port =
        port_env != nullptr ? port_env : "8080";

    boost::asio::io_context io;

    tcp::resolver resolver(io);
    tcp::socket socket(io);

    boost::system::error_code ec;

    const auto endpoints = resolver.resolve(host, port, ec);

    if (ec)
    {
        std::cerr << "Address resolution failed: "
                  << ec.message() << '\n';
        return 1;
    }

    boost::asio::connect(socket, endpoints, ec);

    if (ec)
    {
        std::cerr << "Connection failed: "
                  << ec.message() << '\n';
        return 1;
    }

    std::cout << "Connected to "
              << host << ':' << port << '\n';

    std::string request;

    for (;;)
    {
        std::cout << "> ";

        if (!std::getline(std::cin, request))
        {
            break;
        }

        if (request == "exit")
        {
            break;
        }

        request += '\n';

        boost::asio::write(
            socket,
            boost::asio::buffer(request),
            ec);

        if (ec)
        {
            std::cerr << "Write failed: "
                      << ec.message() << '\n';
            return 1;
        }

        std::string response;

        boost::asio::read_until(
            socket,
            boost::asio::dynamic_buffer(response),
            '\n',
            ec);

        if (ec)
        {
            std::cerr << "Read failed: "
                      << ec.message() << '\n';
            return 1;
        }

        std::cout << response;
    }
}