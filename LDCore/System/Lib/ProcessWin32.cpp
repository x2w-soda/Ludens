#include <Ludens/Header/Platform.h>
#ifdef LD_PLATFORM_WIN32
#include <Ludens/System/Process.h>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include <shellapi.h>

namespace LD {

void shell_open(const FS::Path& path)
{
    std::string pathStr = path.string();

    ShellExecuteA(NULL, "open", pathStr.c_str(), NULL, NULL, SW_SHOWNORMAL);
}

} // namespace LD

#endif // LD_PLATFORM_WIN32