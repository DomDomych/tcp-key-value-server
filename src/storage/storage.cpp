#include "storage/storage.hpp"

#include <utility>

Storage::Storage(std::string connection_string)
    : database_(std::move(connection_string))
{
}

std::optional<std::string> Storage::get(std::string_view key)
{
    return database_.get(key);
}

bool Storage::set(std::string_view key, std::string_view value)
{
    return database_.set(key, value);
}

bool Storage::del(std::string_view key)
{
    return database_.del(key);
}