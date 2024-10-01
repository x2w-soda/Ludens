#include <memory>
#include "Core/Header/Include/Platform.h"
#include "Core/Header/Include/Error.h"
#include "Core/OS/Include/ConditionVariable.h"
#include "Core/OS/Include/Mutex.h"

#ifdef LD_PLATFORM_WIN32
# include <Windows.h>
#else
# error "not implemented yet"
#endif

namespace LD
{

struct ConditionVariableImpl
{
#ifdef LD_PLATFORM_WIN32
    CONDITION_VARIABLE CV;
#endif
};

ConditionVariable::ConditionVariable()
{
    LD_STATIC_ASSERT(sizeof(mImplData) >= sizeof(ConditionVariableImpl));

    ConditionVariableImpl* data = reinterpret_cast<ConditionVariableImpl*>(mImplData);
    mImpl = new (data) ConditionVariableImpl();

#ifdef LD_PLATFORM_WIN32
    InitializeConditionVariable(&mImpl->CV);
#endif
}

ConditionVariable::~ConditionVariable()
{
#ifdef LD_PLATFORM_WIN32
    // NOP
#endif
    
    mImpl->~ConditionVariableImpl();
}

void ConditionVariable::Wait(Mutex& mutex)
{
#ifdef LD_PLATFORM_WIN32
    SleepConditionVariableCS(&mImpl->CV, (CRITICAL_SECTION*)mutex.GetNativeHandle(), INFINITE);
#endif
}

void ConditionVariable::SignalOne()
{
#ifdef LD_PLATFORM_WIN32
    WakeConditionVariable(&mImpl->CV);
#endif
}

void ConditionVariable::SignalAll()
{
#ifdef LD_PLATFORM_WIN32
    WakeAllConditionVariable(&mImpl->CV);
#endif
}

} // namespace LD