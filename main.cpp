#include "server/server.hpp"
#include <boost/asio.hpp>

int main()
{
    boost::asio::io_context io;

    Server server(io, 8080);

    server.start();
    io.run();

    return 0;
}