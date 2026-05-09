#include <Ludens/Header/Platform.h>
#ifdef LD_PLATFORM_LINUX
#include <Ludens/DSA/String.h>
#include <Ludens/DSA/Vector.h>
#include <Ludens/System/Process.h>

#include <spawn.h>
#include <sys/wait.h>
#include <unistd.h>

#include <atomic>
#include <thread>

#include "LineBuffer.h"

namespace LD {

void shell_open(const FS::Path& path)
{
    std::string cmd = "xdg-open \"" + path.string() + "\" &";
    system(cmd.c_str());
}

struct ProcessObj
{
    std::atomic_bool isRunning = false;
    std::thread stderrThread;
    std::thread stdoutThread;
    LineBuffer stderrLines;
    LineBuffer stdoutLines;
    ProcessLineFn onStdoutLine = nullptr;
    ProcessLineFn onStderrLine = nullptr;
    pid_t pid = {};
    int stdoutFD = -1;
    int stderrFD = -1;

    void read_fd(int fd, LineBuffer& lineBuf);
    void terminate();
    void close_fds();
    void join_threads();

    static bool try_create(ProcessObj* obj, const ProcessInfo& info);
};

void ProcessObj::read_fd(int fd, LineBuffer& lineBuf)
{
    char8_t buffer[BUFSIZ];
    String pending;

    while (isRunning.load())
    {
        ssize_t n = read(fd, buffer, BUFSIZ - 1);

        if (n <= 0)
            break;

        buffer[n] = '\0';
        pending.append(buffer, n);

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
    if (!isRunning.load())
        return;

    kill(pid, SIGTERM);
    pid = 0;

    isRunning.store(false);
}

void ProcessObj::close_fds()
{
    if (stdoutFD != -1)
    {
        close(stdoutFD);
        stdoutFD = -1;
    }

    if (stderrFD != -1)
    {
        close(stderrFD);
        stderrFD = -1;
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
    int stdoutPipe[2];
    int stderrPipe[2];

    if (pipe(stdoutPipe) != 0)
        return false;

    if (pipe(stderrPipe) != 0)
    {
        close(stdoutPipe[0]);
        close(stdoutPipe[1]);
        return false;
    }

    obj->pid = fork();

    if (obj->pid < 0) // failure
    {
        close(stdoutPipe[0]);
        close(stdoutPipe[1]);
        close(stderrPipe[0]);
        close(stderrPipe[1]);
        return false;
    }

    if (obj->pid == 0) // child process
    {
        dup2(stdoutPipe[1], STDOUT_FILENO);
        dup2(stderrPipe[1], STDERR_FILENO);

        close(stdoutPipe[0]);
        close(stdoutPipe[1]);
        close(stderrPipe[0]);
        close(stderrPipe[1]);

        std::string exec = info.executable.string();
        Vector<char*> argv(info.args.size() + 2);
        argv[0] = exec.data();

        for (size_t i = 1; i <= info.args.size(); i++)
            argv[i] = (char*)info.args[i - 1].data();

        argv.back() = nullptr;

        execvp(argv[0], argv.data());

        _exit(EXIT_FAILURE); // execvp failed
    }

    // parent process
    close(stdoutPipe[1]);
    close(stderrPipe[1]);

    obj->onStdoutLine = info.onStdoutLine;
    obj->onStderrLine = info.onStderrLine;
    obj->stdoutFD = stdoutPipe[0];
    obj->stderrFD = stderrPipe[0];
    obj->isRunning.store(true);
    obj->stdoutThread = std::thread([obj]() { obj->read_fd(obj->stdoutFD, obj->stdoutLines); });
    obj->stderrThread = std::thread([obj]() { obj->read_fd(obj->stderrFD, obj->stderrLines); });

    return obj;
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
    obj->close_fds();
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

#endif // LD_PLATFORM_LINUX
