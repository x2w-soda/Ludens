#include <memory>
#include "Core/OS/Include/Mutex.h"
#include "Core/Header/Include/Platform.h"
#include "Core/Header/Include/Error.h"

#ifdef LD_PLATFORM_WIN32
# include <windows.h>
#else
# error "not implemented yet"
#endif

namespace LD
{

struct MutexImpl
{
#ifdef LD_PLATFORM_WIN32
    CRITICAL_SECTION CS;
#endif
};

Mutex::Mutex()
{
    LD_STATIC_ASSERT(sizeof(mImplData) >= sizeof(MutexImpl));

    MutexImpl* data = reinterpret_cast<MutexImpl*>(mImplData);
    mImpl = new (data) MutexImpl();

#ifdef LD_PLATFORM_WIN32
    InitializeCriticalSection(&mImpl->CS);
#endif
}

Mutex::~Mutex()
{
#ifdef LD_PLATFORM_WIN32
    DeleteCriticalSection(&mImpl->CS);
#endif

    mImpl->~MutexImpl();
}

void Mutex::Lock()
{
#ifdef LD_PLATFORM_WIN32
    EnterCriticalSection(&mImpl->CS);
#endif
}

void Mutex::Unlock()
{
#ifdef LD_PLATFORM_WIN32
    LeaveCriticalSection(&mImpl->CS);
#endif
}

void* Mutex::GetNativeHandle()
{
#ifdef LD_PLATFORM_WIN32
    return (void*)&mImpl->CS;
#endif
}

} // namespace LD