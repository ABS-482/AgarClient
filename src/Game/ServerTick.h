#pragma once

#include <atomic>
#include <cstdint>

class ServerTick
{
public:
    uint64_t current() const noexcept
    {
        return tick.load(std::memory_order_acquire);
    }

    void advance() noexcept
    {
        tick.fetch_add(
            1,
            std::memory_order_release
        );
    }

private:
    std::atomic<uint64_t> tick{ 0 };
};