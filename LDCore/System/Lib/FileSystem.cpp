#include <Ludens/Profiler/Profiler.h>
#include <Ludens/System/FileSystem.h>
#include <algorithm>
#include <cstring>
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

bool read_file_to_vector(const FS::Path& path, std::vector<byte>& v)
{
    LD_PROFILE_SCOPE;

    uint64_t fileSize;
    if (!read_file(path, fileSize, nullptr) || fileSize == 0)
    {
        v.clear();
        return false;
    }

    v.resize(fileSize);
    return read_file(path, fileSize, v.data());
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

bool is_directory(const Path& path)
{
    return fs::exists(path) && fs::is_directory(path);
}

void filter_files_by_extension(std::vector<FS::Path>& paths, const char* extension)
{
    if (!extension || strlen(extension) == 0)
        return;

    while (extension[0] == '.')
        extension++;

    std::string filterExt(extension);

    paths.erase(std::remove_if(
                    paths.begin(), paths.end(),
                    [&](const FS::Path& path) {
                        if (!fs::is_regular_file(path))
                            return false;

                        std::string pathExt = path.extension().string();
                        if (pathExt.empty())
                            return true;

                        if (pathExt.starts_with("."))
                            pathExt = pathExt.substr(1);

                        return pathExt != filterExt;
                    }),
                paths.end());
}

} // namespace FS
} // namespace LD
