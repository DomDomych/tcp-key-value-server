#pragma once

#include <mutex>
#include <optional>
#include <shared_mutex>
#include <string>
#include <string_view>
#include <unordered_map>
#include <pqxx/pqxx>

class Storage
{
  public:
    Storage(std::string connection_string);

    std::optional<std::string> get(std::string_view key);
    bool set(std::string_view key, std::string_view value);
    bool del(std::string_view key);

  private:
    PostgresStorage database_;
    LruCache cache_;
};

class PostgresStorage
{
  public:
    PostgresStorage(std::string connection_string);

    std::optional<std::string> get(std::string_view key);
    bool set(std::string_view key, std::string_view value);
    bool del(std::string_view key);

  private:
    pqxx::connection connection_;
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
  // other incapsulated stuff
};