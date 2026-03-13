#include <Ludens/Header/Platform.h>
#ifdef LD_PLATFORM_LINUX
#include <Ludens/System/Process.h>

#include <spawn.h>
#include <sys/wait.h>
#include <unistd.h>

namespace LD {

void shell_open(const FS::Path& path)
{
    std::string cmd = "xdg-open \"" + path.string() + "\" &";
    system(cmd.c_str());
}

} // namespace LD

#endif // LD_PLATFORM_LINUX
