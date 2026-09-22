#include "storage/storage.hpp"

#include <stdexcept>
#include <utility>

PostgresStorage::PostgresStorage(std::string connection_string,std::size_t pool_size)
    :connection_pool_(std::move(connection_string),pool_size)
{
    if (pool_size == 0)
    {
        throw std::invalid_argument("Connection pool size must be greater than zero");
    }
}

std::optional<std::string> PostgresStorage::get(std::string_view key)
{

    auto connection = connection_pool_.acquire();

    pqxx::read_transaction transaction{connection.get()};

    auto result =
        transaction.exec("SELECT value FROM kv_store WHERE key = $1", pqxx::params{key});

    if (result.empty())
    {
        return std::nullopt;
    }

    return result[0]["value"].as<std::string>();
}

bool PostgresStorage::set(std::string_view key, std::string_view value)
{

    try
    {

        auto connection = connection_pool_.acquire();

        pqxx::work transaction{connection.get()};

        transaction.exec("INSERT INTO kv_store (key, value)"
                                "VALUES ($1, $2) "
                                "ON CONFLICT (key) "
                                "DO UPDATE SET value = EXCLUDED.value",
                                pqxx::params{key, value});

        transaction.commit();

        return true;
    }
    catch (const std::exception &)
    {
        return false;
    }
}

bool PostgresStorage::del(std::string_view key)
{

    try
    {

        auto connection = connection_pool_.acquire();
        pqxx::work transaction{connection.get()};

        auto result =
            transaction.exec("DELETE FROM kv_store WHERE key = $1", pqxx::params{key});

        const bool deleted = result.affected_rows() != 0;

        transaction.commit();

        return deleted;
    }
    catch (const std::exception &)
    {
        return false;
    }
}