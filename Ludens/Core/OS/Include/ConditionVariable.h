#pragma once

#include "Core/Header/Include/Types.h"

namespace LD
{

class Mutex;

class ConditionVariable
{
public:
    ConditionVariable();
    ConditionVariable(const ConditionVariable&) = delete;
    ~ConditionVariable();

    ConditionVariable& operator=(const ConditionVariable&) = delete;

    void Wait(Mutex& mutex);

    void SignalOne();

    void SignalAll();

private:
    struct ConditionVariableImpl* mImpl;
    alignas(16) u8 mImplData[64];
};

} // namespace LD