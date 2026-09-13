#pragma once

#include <unordered_map>
#include <shared_mutex>
#include <optional>
#include <string>
#include <string_view>


class Storage
{
    public:
        void set(std::string_view key,std::string_view value);
        std::optional<std::string>  get(std::string_view key) const;
        bool del(std::string_view key);

    private:
        std::unordered_map<std::string,std::string> storage_;
        mutable std::shared_mutex mutex_;
};