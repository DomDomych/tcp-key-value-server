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

bool PostgresStorage::set(std::string_view key,std::string_view value)
{
    std::lock_guard{mutex_};

    try
    {
        pqxx::work transaction{connection_};

        transaction.exec(
            "INSERT INTO kv_store (key, value)"
            "VALUES ($1, $2) "
            "ON CONFLICT (key) "
            "DO UPDATE SET value = EXCLUDED.value",
            pqxx::params{transaction,key}
        );

        transaction.commit();

        return true;
    }
    catch(const std::exception& e)
    {
        return false;
    }
    
}

bool PostgresStorage::del(std::string_view key)
{
    std::lock_guard lock{mutex_};

    try
    {
        pqxx::work transaction{connection_};

        auto result = transaction.exec(
            "DELETE FROM kv_store WHERE key = $1",
            pqxx::params{transaction, key});

        const bool deleted = result.affected_rows() != 0;

        transaction.commit();

        return deleted;
    }
    catch (const std::exception &)
    {
        return false;
    }
}