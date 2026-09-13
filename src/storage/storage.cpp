#include "storage/storage.hpp"

void Storage::set(std::string_view key, std::string_view value)
{
    std::unique_lock lock(mutex_);
    storage_[std::string(key)] = std::string(value);

    return;
}

std::optional<std::string> Storage::get(std::string_view key) const
{
    std::shared_lock lock(mutex_);
    auto it = storage_.find(std::string(key));

    if (it == storage_.end())
    {
        return std::nullopt;
    }

    return it->second;
}

bool Storage::del(std::string_view key)
{
    std::unique_lock lock(mutex_);

    return storage_.erase(std::string(key)) != 0;
}