#pragma once

#include "connection_pool.hpp"
#include <array>
#include <mutex>
#include <optional>
#include <pqxx/pqxx>
#include <shared_mutex>
#include <string>
#include <string_view>
#include <unordered_map>

class PostgresStorage
{
  public:
    PostgresStorage(std::string connection_string, std::size_t pool_size);

    std::optional<std::string> get(std::string_view key);
    bool set(std::string_view key, std::string_view value);
    bool del(std::string_view key);

  private:
    ConnectionPool connection_pool_;
    std::mutex mutex_;
};

class LruCache
{

  public:
    LruCache(std::size_t capacity);

    std::optional<std::string> get(std::string_view key);

    void put(std::string key, std::string value);

    void erase(std::string_view key);

  private:
    std::size_t capacity_;

    std::list<std::pair<std::string, std::string>> items_;

    std::unordered_map<std::string, std::list<std::pair<std::string, std::string>>::iterator>
        index_;

    std::mutex mutex_;
};

class Storage
{
  public:
    Storage(std::string connection_string, std::size_t capacity, std::size_t size);

    std::optional<std::string> get(std::string_view key);
    bool set(std::string_view key, std::string_view value);
    bool del(std::string_view key);

  private:
    PostgresStorage database_;
    LruCache cache_;

    std::array<std::shared_mutex, 64> mutexes_;

    std::shared_mutex &mutex_for(std::string_view key);
};
