#include <storage/storage.hpp>

Storage::Storage(std::string connection_string, std::size_t cache_capacity, std::size_t pool_size)
    : database_(std::move(connection_string), pool_size), cache_(cache_capacity)
{
}

std::shared_mutex &Storage::mutex_for(std::string_view key)
{
    std::size_t hash = std::hash<std::string_view>{}(key);

    return mutexes_[hash % 64];
}

std::optional<std::string> Storage::get(std::string_view key)
{

    auto &mutex = mutex_for(key);

    std::shared_lock lock(mutex);

    if (auto value = cache_.get(key))
    {
        return value;
    }

    auto value = database_.get(key);

    if (value)
    {
        cache_.put(std::string(key), *value);
    }

    return value;
}

bool Storage::set(std::string_view key, std::string_view value)
{
    auto &mutex = mutex_for(key);

    std::unique_lock lock(mutex);

    if (!database_.set(key, value))
    {
        return false;
    }

    cache_.put(std::string(key), std::string(value));

    return true;
}

bool Storage::del(std::string_view key)
{
    auto &mutex = mutex_for(key);

    std::unique_lock lock(mutex);

    if (!database_.del(key))
    {
        return false;
    }

    cache_.erase(key);

    return true;
}