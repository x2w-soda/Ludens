#pragma once

#include "Core/Header/Include/Types.h"

namespace LD
{

typedef int (*ThreadFunction)(int tid, void* userdata);

class Thread
{
public:
    Thread();
    Thread(const Thread&) = delete;
    ~Thread();

    Thread& operator=(const Thread&) = delete;

    /// spawn and run a thread
    void Run(ThreadFunction entry, void* userdata);

    /// stop and join the thread
    void Stop();

    bool IsRunning();

    /// get OS assigned thread ID 
    int GetID();

private:
    struct ThreadImpl* mImpl;
    alignas(16) u8 mImplData[128];
};

} // namespace LD