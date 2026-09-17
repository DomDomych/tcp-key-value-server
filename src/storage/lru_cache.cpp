#include <storage/storage.hpp>

LruCache::LruCache(std::size_t capacity) : capacity_(capacity) {};

std::optional<std::string> LruCache::get(std::string_view key)
{
    std::lock_guard lock(mutex_);

    auto it = index_.find(std::string(key));

    if (it == index_.end())
    {
        return std::nullopt;
    }

    items_.splice(items_.begin(), items_, it->second);

    return it->second->second;
}

void LruCache::put(std::string key, std::string value)
{

    if (capacity_ == 0)
        return;
    std::lock_guard lock(mutex_);

    auto it = index_.find(key);

    if (it != index_.end())
    {
        it->second->second = std::move(value);

        items_.splice(items_.begin(), items_, it->second);

        return;
    }

    items_.emplace_front(std::move(key), std::move(value));

    index_[items_.front().first] = items_.begin();

    if (items_.size() > capacity_)
    {
        const std::string &old_key = items_.back().first;

        index_.erase(old_key);
        items_.pop_back();
    }
}

void LruCache::erase(std::string_view key)
{
    std::lock_guard lock(mutex_);

    auto it = index_.find(std::string(key));

    if (it == index_.end())
    {
        return;
    }

    items_.erase(it->second);
    index_.erase(it);
}