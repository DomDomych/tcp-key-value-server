#pragma once

#include <atomic>
#include <cstdint>

class Metrics
{
    public:
        void inc_get_requests();
        void inc_set_requests();
        void inc_del_requests();

        std::uint64_t get_requests() const;
        std::uint64_t set_requests() const;
        std::uint64_t del_requests() const;

    private:
        std::atomic<std::uint64_t> get_requests_{0};
        std::atomic<std::uint64_t> set_requests_{0};
        std::atomic<std::uint64_t> del_requests_{0};
};