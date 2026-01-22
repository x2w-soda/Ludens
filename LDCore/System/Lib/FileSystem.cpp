#include <Ludens/DSA/Diagnostics.h>
#include <Ludens/Profiler/Profiler.h>
#include <Ludens/System/FileSystem.h>

#include <algorithm>
#include <cstring>
#include <format>
#include <fstream>

namespace fs = std::filesystem;

namespace LD {
namespace FS {

Path current_path()
{
    return fs::current_path();
}

bool get_directory_content(const Path& directory, Vector<Path>& contents, std::string& err)
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

bool get_file_size(const Path& path, uint64_t& size, std::string& err)
{
    err.clear();

    try
    {
        size = (uint64_t)fs::file_size(path);
    }
    catch (fs::filesystem_error& e)
    {
        err = e.what();
    }

    return err.empty();
}

bool get_file_size(const Path& path, uint64_t& size, Diagnostics& diag)
{
    DiagnosticScope scope(diag, "get_file_size");

    std::string err;
    if (!get_file_size(path, size, err))
    {
        diag.mark_error(err);
        return false;
    }

    return true;
}

bool get_positive_file_size(const Path& path, uint64_t& size, Diagnostics& diag)
{
    DiagnosticScope scope(diag, "get_positive_file_size");

    if (!get_file_size(path, size, diag))
        return false;

    if (size == 0)
    {
        diag.mark_error(std::format("file [{}] is empty", path.string()));
        return false;
    }

    return true;
}

uint64_t read_file(const Path& path, const MutView& view, std::string& err)
{
    LD_PROFILE_SCOPE;

    if (!view.data)
    {
        err = "cant write to null view data";
        return false;
    }

    if (!fs::exists(path))
    {
        err = std::format("file path [{}] does not exist", path.string());
        return false;
    }

    std::ifstream file(path, std::ios::binary);

    if (!file.is_open())
    {
        err = std::format("failed to open file [{}]", path.string());
        return false;
    }

    file.seekg(0, std::ios::end);
    std::streamsize fsize = file.tellg();

    if (view.size < (uint64_t)fsize)
    {
        err = std::format("cant write to view of size {}, file size is {}", view.size, fsize);
        file.close();
        return false;
    }

    file.seekg(0, std::ios::beg);
    file.read(view.data, fsize);
    file.close();

    return (uint64_t)fsize;
}

uint64_t read_file(const Path& path, const MutView& view, Diagnostics& diag)
{
    std::string err;
    DiagnosticScope scope(diag, __func__);
    uint64_t readSize = read_file(path, view, err);

    if (readSize == 0)
    {
        diag.mark_error(err);
        return 0;
    }

    return readSize;
}

bool read_file_to_vector(const FS::Path& path, Vector<byte>& v, std::string& err)
{
    LD_PROFILE_SCOPE;

    uint64_t fileSize = 0;
    if (!get_file_size(path, fileSize, err))
        return false;

    // note that empty file of size 0 is not an error.
    v.resize(fileSize);

    if (fileSize > 0)
        read_file(path, MutView((char*)v.data(), fileSize), err);

    return err.empty();
}

bool write_file(const Path& path, const View& view, std::string& err)
{
    LD_PROFILE_SCOPE;

    if (!view.data || view.size == 0)
    {
        err = std::format("no data to write to [{}]", path.string());
        return false;
    }

    std::ofstream file(path, std::ios::binary);

    if (!file.is_open())
    {
        err = std::format("failed to open file [{}]", path.string());
        return false;
    }

    file.write(view.data, view.size);
    file.close();

    return true;
}

bool write_file(const Path& path, const View& view, Diagnostics& diag)
{
    std::string err;
    DiagnosticScope scope(diag, __func__);

    if (!write_file(path, view, err))
    {
        diag.mark_error(err);
        return false;
    }

    return true;
}

bool write_file_and_swap_backup(const Path& path, const View& view, std::string& err)
{
    LD_PROFILE_SCOPE;

    if (!FS::exists(path))
        return FS::write_file(path, view, err);

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

    if (!FS::write_file(tmpPath, view, err))
        return false;

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

bool remove(const FS::Path& path, std::string& err)
{
    try
    {
        fs::remove(path);
    }
    catch (const std::filesystem::filesystem_error& e)
    {
        err = std::format("failed to remove [{}]\nfilesystem_error: {}", path.string(), e.what());
        return false;
    }

    return true;
}

void filter_files_by_extension(Vector<FS::Path>& paths, const char* extension)
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
