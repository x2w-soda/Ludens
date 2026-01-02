#include <Ludens/Profiler/Profiler.h>
#include <Ludens/System/FileSystem.h>
#include <algorithm>
#include <cstring>
#include <format>
#include <fstream>

namespace LD {
namespace FS {

namespace fs = std::filesystem;

Path current_path()
{
    return fs::current_path();
}

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
    if (!fs::exists(path))
        return 0;

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

bool write_file_and_swap_backup(const Path& path, uint64_t size, const byte* buf, std::string& err)
{
    LD_PROFILE_SCOPE;

    if (!FS::exists(path))
    {
        if (!FS::write_file(path, size, buf))
        {
            err = std::format("failed to write_file to [{}]", path.string());
            return false;
        }

        return true;
    }

    // 1. rename existing file as backup
    FS::Path backupPath = path;
    FS::Path backupExt = path.has_extension() ? FS::Path(".bak" + path.extension().string()) : FS::Path(".bak");
    backupPath.replace_extension(backupExt);

    try
    {
        fs::rename(path, backupPath);
    }
    catch (const std::filesystem::filesystem_error& e)
    {
        err = std::format("failed to rename [{}] to [{}]\nfilesystem_error: {}", path.string(), backupPath.string(), e.what());
        return false;
    }

    // 2. write new contents to temporary file
    FS::Path tmpPath = path;
    FS::Path tmpExt = path.has_extension() ? FS::Path(".tmp" + path.extension().string()) : FS::Path(".tmp");
    tmpPath.replace_extension(tmpExt);

    if (!FS::write_file(tmpPath, size, buf))
    {
        err = std::format("failed to write new contents to [{}]", tmpPath.string());
        return false;
    }

    // 3. rename temporary file to destination save path
    try
    {
        fs::rename(tmpPath, path);
    }
    catch (const std::filesystem::filesystem_error& e)
    {
        err = std::format("failed to rename [{}] to [{}]\nfilesystem_error: {}", tmpPath.string(), path.string(), e.what());
        return false;
    }

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
