#pragma once

#include <chrono>
#include <cstddef>

namespace LD {

class Timer
{
public:
    Timer() = default;
    Timer(const Timer&) = delete;

    Timer& operator=(const Timer&) = delete;

    /// @brief start the timer
    void start();

    /// @brief stop the timer
    /// @return output duration in micro seconds
    size_t stop();

private:
    std::chrono::high_resolution_clock::time_point mBegin;
};

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