#include "metrics/metrics.hpp"

void Metrics::inc_get_requests()
{
    get_requests_.fetch_add(1, std::memory_order_relaxed);
}

void Metrics::inc_set_requests()
{
    set_requests_.fetch_add(1, std::memory_order_relaxed);
}

void Metrics::inc_del_requests()
{
    del_requests_.fetch_add(1, std::memory_order_relaxed);
}

std::uint64_t Metrics::get_requests() const
{
    return get_requests_.load(std::memory_order_relaxed);
}

std::uint64_t Metrics::set_requests() const
{
    return set_requests_.load(std::memory_order_relaxed);
}

std::uint64_t Metrics::del_requests() const
{
    return del_requests_.load(std::memory_order_relaxed);
}

