#include <memory>
#include <iostream>
#include "Core/Header/Include/Platform.h"
#include "Core/Header/Include/Error.h"
#include "Core/OS/Include/Thread.h"

#ifdef LD_PLATFORM_WIN32
#include <windows.h>
#include <process.h>
#else
#error "not implemented"
#endif

namespace LD
{

struct ThreadImpl
{
    bool IsRunning;
    int ExitCode;
    int ID;
    ThreadFunction Entry;
    void* UserData;
#ifdef LD_PLATFORM_WIN32
    HANDLE Handle;
#endif
};

#ifdef LD_PLATFORM_WIN32
static DWORD WINAPI ThreadFunctionImpl(void* arg)
{
    ThreadImpl* impl = (ThreadImpl*)arg;

    return (DWORD)impl->Entry(impl->ID, impl->UserData);
}
#endif

Thread::Thread()
{
    LD_STATIC_ASSERT(sizeof(mImplData) >= sizeof(ThreadImpl));

    ThreadImpl* data = reinterpret_cast<ThreadImpl*>(mImplData);
    mImpl = new (data) ThreadImpl();
    mImpl->IsRunning = false;
    mImpl->ID = -1;
    mImpl->ExitCode = 0;
    mImpl->Entry = nullptr;
#ifdef LD_PLATFORM_WIN32
    mImpl->Handle = INVALID_HANDLE_VALUE;
#endif
}

Thread::~Thread()
{
    if (mImpl->IsRunning)
        Stop();

    mImpl->~ThreadImpl();
}

void Thread::Run(ThreadFunction entry, void* userdata)
{
    mImpl->Entry = entry;
    mImpl->UserData = userdata;

#ifdef LD_PLATFORM_WIN32
    mImpl->Handle = CreateThread(NULL, 0, ThreadFunctionImpl, mImpl, 0, NULL);
    if (mImpl->Handle == NULL)
    {
        std::cerr << "Win32 CreateThread failed: GetLastError() = " << GetLastError() << std::endl;
        return;
    }

    mImpl->ID = GetThreadId(mImpl->Handle);
    if (mImpl->ID == 0)
    {
        std::cerr << "Win32 GetThreadID failed: GetLastError() = " << GetLastError() << std::endl;
        return;
    }
#endif

    mImpl->IsRunning = true;
}

void Thread::Stop()
{
    LD_DEBUG_ASSERT(mImpl->IsRunning);

    if (!mImpl->IsRunning)
        return;

#ifdef LD_PLATFORM_WIN32
    WaitForSingleObject(mImpl->Handle, INFINITE);
    GetExitCodeThread(mImpl->Handle, (DWORD*)&mImpl->ExitCode);
    CloseHandle(mImpl->Handle);
    mImpl->Handle = INVALID_HANDLE_VALUE;
#endif

    mImpl->IsRunning = false;
}

bool Thread::IsRunning()
{
    return mImpl->IsRunning;
}

int Thread::GetID()
{
    LD_DEBUG_ASSERT(mImpl->IsRunning);

    return mImpl->ID;
}


} // namespace LD