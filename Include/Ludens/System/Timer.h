#pragma once

#include <chrono>

namespace LD {

class ScopeTimer
{
public:
    ScopeTimer() = delete;

    /// @brief begin timing until the timer goes out of scope
    /// @param us output duration in micro seconds
    ScopeTimer(size_t* us);
    ScopeTimer(const ScopeTimer&) = delete;
    ~ScopeTimer();

    ScopeTimer& operator=(const ScopeTimer&) = delete;

private:
    std::chrono::high_resolution_clock::time_point mBegin;
    size_t* mUS;
};

} // namespace LD