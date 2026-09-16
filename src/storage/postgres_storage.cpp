#include "storage/storage.hpp"

#include <stdexcept>
#include <utility>

PostgresStorage::PostgresStorage(std::string connection_string)
    : connection_(std::move(connection_string))
{
    if(!connection_.is_open())
    {
        throw std::runtime_error("Failed to connect to PostgresSQL");
    }
}

std::optional<std::string> PostgresStorage::get(std::string_view key)
{
    std::lock_guard{mutex_};

    pqxx::read_transaction transaction{connection_};

    auto result = transaction.exec(
        "SELECT value FROM kv_store WHERE key = $1",
        pqxx::params{transaction,key});
    
    if(result.empty())
    {
        return std::nullopt;
    }

    return result[0]["value"].as<std::string>();
}


