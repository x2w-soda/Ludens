#include <Ludens/System/Timer.h>

namespace LD {

void Timer::start()
{
    mBegin = std::chrono::high_resolution_clock::now();
}

size_t Timer::stop()
{
    std::chrono::high_resolution_clock::time_point end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - mBegin);

    return duration.count();
}

ScopeTimer::ScopeTimer(size_t* us)
    : mBegin(std::chrono::high_resolution_clock::now()), mUS(us)
{
}

ScopeTimer::~ScopeTimer()
{
    std::chrono::high_resolution_clock::time_point end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - mBegin);

    if (mUS)
        *mUS = (size_t)duration.count();
}

} // namespace LD