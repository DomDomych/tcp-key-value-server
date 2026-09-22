#include "storage/connection_pool.hpp"

#include <utility>

ConnectionGuard::ConnectionGuard(ConnectionPool& pool,
                std::unique_ptr<pqxx::connection> connection):pool_(pool),connection_(std::move(connection))
                {}
ConnectionGuard::~ConnectionGuard()
{
    if(connection_)
    {
        pool_.release(std::move(connection_));
    }
}

ConnectionGuard::ConnectionGuard(ConnectionGuard&& other) noexcept
    : pool_(other.pool_),
      connection_(std::move(other.connection_))
{
}

pqxx::connection& ConnectionGuard::get()
{
    return *connection_;
}

ConnectionPool::ConnectionPool(const std::string& connection_string,
                               std::size_t size)
{
    available_.reserve(size);

    for(std::size_t i=0;i<size;i++)
    {
        available_.push_back(
            std::make_unique<pqxx::connection>(connection_string));
    }

}

ConnectionGuard ConnectionPool::acquire()
{
    std::unique_lock lock(mutex_);

    cv_.wait(lock,[this]
    {
        return !available_.empty();
    });

    auto connection = std::move(available_.back());
    available_.pop_back();

    return ConnectionGuard(*this,std::move(connection));
}
    
void ConnectionPool::release(
    std::unique_ptr<pqxx::connection> connection
)
{
    {
        std::lock_guard lock(mutex_);
        available_.push_back(std::move(connection));
    }

    cv_.notify_one();
}
