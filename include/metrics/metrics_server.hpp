#pragma once

#include "metrics.hpp"
#include <boost/asio.hpp>
#include <cstdint>

class MetricsServer
{
  public:
    MetricsServer(boost::asio::io_context &io_context, std::uint16_t port,Metrics& metrics);

    void start();
    void stop();

  private:
    void accept();

    boost::asio::ip::tcp::acceptor acceptor_;
    Metrics& metrics_;

};