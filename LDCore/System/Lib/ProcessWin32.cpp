#include <Ludens/Header/Platform.h>
#ifdef LD_PLATFORM_WIN32
#include <Ludens/System/Process.h>

#include <atomic>
#include <cstdio> // BUFSIZ
#include <thread>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include <shellapi.h>

#include "LineBuffer.h"

namespace LD {

void shell_open(const FS::Path& path)
{
    std::string pathStr = path.string();

    ShellExecuteA(NULL, "open", pathStr.c_str(), NULL, NULL, SW_SHOWNORMAL);
}

/// @brief Win32 process implementation.
struct ProcessObj
{
    HANDLE handle = nullptr;
    HANDLE stdoutPipe = nullptr;
    HANDLE stderrPipe = nullptr;
    std::atomic_bool isRunning = false;
    std::thread stderrThread;
    std::thread stdoutThread;
    LineBuffer stderrLines;
    LineBuffer stdoutLines;
    ProcessLineFn onStdoutLine = nullptr;
    ProcessLineFn onStderrLine = nullptr;

    void read_pipe(HANDLE pipe, LineBuffer& buf);
    void terminate();
    void close_pipes();
    void join_threads();

    static bool try_create(ProcessObj* obj, const ProcessInfo& info);
};

void ProcessObj::read_pipe(HANDLE pipe, LineBuffer& lineBuf)
{
    char8_t buffer[BUFSIZ];
    String pending;

    while (isRunning.load())
    {
        DWORD bytesRead = 0;
        BOOL ok = ReadFile(
            pipe,
            buffer,
            BUFSIZ - 1,
            &bytesRead,
            nullptr);

        if (!ok || bytesRead == 0)
            break;

        buffer[bytesRead] = '\0';
        pending.append(buffer, bytesRead);

        size_t pos = 0;

        while ((pos = pending.find('\n')) != String::npos)
        {
            String line = pending.substr(0, pos);

            if (!line.empty() && line.back() == '\r')
                line.pop_back();

            lineBuf.append(line);

            pending.erase(0, pos + 1);
        }
    }
}

void ProcessObj::terminate()
{
    if (!handle || !isRunning.load())
        return;

    (void)TerminateProcess(handle, 1);
    handle = nullptr;

    isRunning.store(false);
}

void ProcessObj::close_pipes()
{
    if (stdoutPipe)
    {
        CloseHandle(stdoutPipe);
        stdoutPipe = nullptr;
    }

    if (stderrPipe)
    {
        CloseHandle(stderrPipe);
        stderrPipe = nullptr;
    }
}

void ProcessObj::join_threads()
{
    if (stdoutThread.joinable())
        stdoutThread.join();

    if (stderrThread.joinable())
        stderrThread.join();
}

bool ProcessObj::try_create(ProcessObj* obj, const ProcessInfo& info)
{
    SECURITY_ATTRIBUTES sa{};
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = nullptr;

    HANDLE stdoutR = nullptr;
    HANDLE stdoutW = nullptr;
    HANDLE stderrR = nullptr;
    HANDLE stderrW = nullptr;

    if (!CreatePipe(&stdoutR, &stdoutW, &sa, 0))
        return false;

    if (!SetHandleInformation(stdoutR, HANDLE_FLAG_INHERIT, 0))
        return false;

    if (!CreatePipe(&stderrR, &stderrW, &sa, 0))
        return false;

    if (!SetHandleInformation(stderrR, HANDLE_FLAG_INHERIT, 0))
        return false;

    STARTUPINFOA si{};
    si.cb = sizeof(si);
    si.dwFlags |= STARTF_USESTDHANDLES;
    si.hStdOutput = stdoutW;
    si.hStdError = stderrW;
    si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);

    PROCESS_INFORMATION pi{};

    std::string cmdLine = info.executable.string();

    for (const String& arg : info.args)
    {
        cmdLine.push_back(' ');
        cmdLine += arg.c_str();
    }

    BOOL ok = CreateProcessA(
        nullptr,
        cmdLine.data(),
        nullptr,
        nullptr,
        TRUE,
        CREATE_NO_WINDOW,
        nullptr,
        nullptr,
        &si,
        &pi);

    CloseHandle(stdoutW);
    CloseHandle(stderrW);

    if (!ok)
    {
        CloseHandle(stdoutR);
        CloseHandle(stderrR);
        return false;
    }

    CloseHandle(pi.hThread);

    obj->onStdoutLine = info.onStdoutLine;
    obj->onStderrLine = info.onStderrLine;

    obj->handle = pi.hProcess;
    obj->stdoutPipe = stdoutR;
    obj->stderrPipe = stderrR;

    obj->isRunning.store(true);

    obj->stdoutThread = std::thread([obj]() { obj->read_pipe(obj->stdoutPipe, obj->stdoutLines); });
    obj->stderrThread = std::thread([obj]() { obj->read_pipe(obj->stderrPipe, obj->stderrLines); });

    return true;
}

//
// Public API
//

Process Process::create(const ProcessInfo& info)
{
    auto* obj = heap_new<ProcessObj>(MEMORY_USAGE_MISC);

    if (!ProcessObj::try_create(obj, info))
    {
        heap_delete<ProcessObj>(obj);
        return {};
    }

    return Process(obj);
}

void Process::destroy(Process process)
{
    auto* obj = process.unwrap();

    obj->terminate();
    obj->close_pipes();
    obj->join_threads();

    heap_delete<ProcessObj>(obj);
}

void Process::poll(void* user)
{
    Vector<String> lines = mObj->stdoutLines.extract();

    if (mObj->onStdoutLine)
    {
        for (const String& line : lines)
            mObj->onStdoutLine(line, user);
    }

    lines = mObj->stderrLines.extract();

    if (mObj->onStderrLine)
    {
        for (const String& line : lines)
            mObj->onStderrLine(line, user);
    }
}

} // namespace LD

#endif // LD_PLATFORM_WIN32