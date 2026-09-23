#include "server/server.hpp"

#include <boost/asio.hpp>
#include <algorithm>
#include <csignal>
#include <iostream>
#include <thread>
#include <vector>

int main()
{
    boost::asio::io_context io;

    Server server(io, 8080);
    server.start();

    boost::asio::signal_set signals(io, SIGINT, SIGTERM);

    signals.async_wait(
        [&server](const boost::system::error_code &ec, int signal_number)
        {
            if (ec)
            {
                return;
            }

            std::cout << "Received signal " << signal_number
                      << ". Shutting down...\n";

            server.stop();
        });

    const std::size_t thread_count =
        std::max(1u, std::thread::hardware_concurrency());

    std::vector<std::thread> workers;
    workers.reserve(thread_count);

    for (std::size_t i = 0; i < thread_count; ++i)
    {
        workers.emplace_back([&io]() { io.run(); });
    }

    for (auto &worker : workers)
    {
        worker.join();
    }

    std::cout << "Server stopped\n";

    return 0;
}