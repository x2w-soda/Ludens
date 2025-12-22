#include "RunTests.h"
#include <Ludens/Log/Log.h>
#include <cstdlib>
#include <filesystem>
#include <string>

namespace fs = std::filesystem;

namespace LD {

static Log sLog("LDBuilder");

void find_test_executables(const char* directory, std::vector<std::string>& paths, const char* extensions)
{
    paths.clear();

    for (const fs::directory_entry& entry : fs::recursive_directory_iterator(fs::path(directory)))
    {
        const fs::path entryPath(entry);

        if (fs::is_directory(entryPath))
            continue;

        // apply path extension filter
        if (extensions)
        {
            if (!entryPath.has_extension() || entryPath.extension().string() != std::string(extensions))
                continue;
        }

        fs::perms permissions = fs::status(entryPath).permissions();
        std::string pathStr = fs::path(entryPath).filename().replace_extension("").string();

        if (pathStr.starts_with("LD") && pathStr.ends_with("Test"))
        {
            paths.push_back(entryPath.string());
        }
    }
}

int run_test_exectuables(std::vector<std::string>& paths)
{
    int counter = 0;

    for (const std::string& path : paths)
    {
        int result = std::system(path.c_str());
        sLog.info("process [{}] returns {}", path, result);

        if (result == 0)
            counter++;
    }

    return counter;
}

} // namespace LD
