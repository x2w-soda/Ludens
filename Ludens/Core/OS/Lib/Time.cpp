#include "Core/OS/Include/Time.h"

namespace LD
{

void Timer::Start()
{
    mStartTime = std::chrono::high_resolution_clock::now();
    mIsRunning = true;
}

void Timer::Stop()
{
    mEndTime = std::chrono::high_resolution_clock::now();
    mIsRunning = false;
}

double Timer::GetMilliSeconds()
{
    std::chrono::time_point<std::chrono::high_resolution_clock> endTime;

    if (mIsRunning)
        endTime = std::chrono::high_resolution_clock::now();
    else
        endTime = mEndTime;

    return std::chrono::duration_cast<std::chrono::microseconds>(endTime - mStartTime).count() / 1000.0;
}

} // namespace LD