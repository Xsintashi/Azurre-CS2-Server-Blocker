#pragma once

#include <mutex>

class Lock {
public:
    Lock() noexcept : lock{ mutex } {}
private:
    std::scoped_lock<std::mutex> lock;
    static inline std::mutex mutex;
};