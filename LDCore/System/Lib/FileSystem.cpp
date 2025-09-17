#include <Ludens/Profiler/Profiler.h>
#include <Ludens/System/FileSystem.h>
#include <fstream>

namespace LD {
namespace FS {

namespace fs = std::filesystem;

bool get_directory_content(const Path& directory, std::vector<Path>& contents, std::string& err)
{
    if (!fs::exists(directory))
    {
        err = directory.string();
        err += "does not exist.";
        return false;
    }

    if (!fs::is_directory(directory))
    {
        err = directory.string();
        err += " is not a directory.";
        return false;
    }

    try
    {
        contents.clear();

        for (const fs::directory_entry& entry : fs::directory_iterator(directory))
        {
            contents.push_back(entry.path());
        }
    }
    catch (const fs::filesystem_error& e)
    {
        err = "fs::filesystem_error: ";
        err += e.what();
        return false;
    }

    return true;
}

uint64_t get_file_size(const Path& path)
{
    return (uint64_t)fs::file_size(path);
}

bool read_file(const Path& path, uint64_t& size, byte* buf)
{
    LD_PROFILE_SCOPE;

    std::ifstream file(path, std::ios::binary);

    if (!fs::exists(path) || !file.is_open())
        return false;

    file.seekg(0, std::ios::end);
    std::streamsize fsize = file.tellg();

    if (buf)
    {
        file.seekg(0, std::ios::beg);
        file.read((char*)buf, fsize);
    }

    file.close();

    size = (uint64_t)fsize;
    return true;
}

bool write_file(const Path& path, uint64_t size, const byte* buf)
{
    LD_PROFILE_SCOPE;

    std::ofstream file(path, std::ios::binary);

    if (!file.is_open())
        return false;

    file.write((const char*)buf, size);
    file.close();

    return true;
}

bool exists(const Path& path)
{
    return fs::exists(path);
}

} // namespace FS
} // namespace LD