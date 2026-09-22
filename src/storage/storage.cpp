#include <storage/storage.hpp>

Storage::Storage(std::string connection_string, std::size_t cache_capacity,std::size_t pool_size)
    : database_(std::move(connection_string),pool_size), cache_(cache_capacity)
{
}

std::optional<std::string> Storage::get(std::string_view key)
{
    std::shared_lock lock(mutex_);

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
    std::unique_lock lock(mutex_);

    if (!database_.set(key, value))
    {
        return false;
    }

    cache_.put(std::string(key), std::string(value));

    return true;
}

bool Storage::del(std::string_view key)
{
    std::unique_lock lock(mutex_);

    if (!database_.del(key))
    {
        return false;
    }

    cache_.erase(key);

    return true;
}