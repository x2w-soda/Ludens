#pragma once

#include "Core/Header/Include/Types.h"

namespace LD
{

class ConditionVariable;

class Mutex
{
    friend class ConditionVariable;

public:
    Mutex();
    Mutex(const Mutex&) = delete;
    ~Mutex();

    Mutex& operator=(const Mutex&) = delete;

    void Lock();

    void Unlock();

private:
    void* GetNativeHandle();

    struct MutexImpl* mImpl;
    alignas(16) u8 mImplData[64];
};

} // namespace LD