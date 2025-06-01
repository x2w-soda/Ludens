#include <Ludens/System/FileSystem.h>
#include <fstream>

namespace LD {
namespace FS {

namespace fs = std::filesystem;

uint64_t get_file_size(const fs::path& path)
{
    return (uint64_t)fs::file_size(path);
}

bool read_file(const std::filesystem::path& path, uint64_t& size, byte* buf)
{
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

bool exists(const std::filesystem::path& path)
{
    return fs::exists(path);
}

} // namespace FS
} // namespace LD