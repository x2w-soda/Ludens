#pragma once

#include <Ludens/DSA/String.h>
#include <Ludens/DSA/Vector.h>
#include <Ludens/Header/Handle.h>
#include <Ludens/System/FileSystem.h>

namespace LD {

/// @brief Opens a file using the system's default application.
/// @param path Path to file on disk.
/// @warning SECURITY: Input path is passed to Win32 ShellExecuteA() or Unix system(),
/// 	     ensure input originates from a trusted source.
void shell_open(const FS::Path& path);

typedef void (*ProcessLineFn)(const String& line, void* user);

struct ProcessInfo
{
    FS::Path executable;
    Vector<String> args;
    ProcessLineFn onStdoutLine = nullptr;
    ProcessLineFn onStderrLine = nullptr;
};

/// @brief Process handle.
struct Process : Handle<struct ProcessObj>
{
    static Process create(const ProcessInfo& info);
    static void destroy(Process process);

    /// @brief Trigger callbacks for reading lines from stdout and stderr.
    void poll(void* user);
};

} // namespace LD