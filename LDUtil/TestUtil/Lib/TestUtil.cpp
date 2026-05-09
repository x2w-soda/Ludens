#include <Ludens/Header/Platform.h>
#include <LudensUtil/TestUtil/TestUtil.h>

namespace fs = std::filesystem;

namespace LD {

TestUtil::TestUtil()
{
    if (!get_root_directory_path(&rootDirPath))
        rootDirPath.clear();
}

bool TestUtil::get_scene_test_dir_path(std::filesystem::path* path)
{
    fs::path rootPath;
    if (!get_root_directory_path(&rootPath))
        return false;

    fs::path dirPath = rootPath / "LDCore/Scene/Test";
    if (!fs::is_directory(dirPath))
        return false;

    if (path)
        *path = fs::absolute(dirPath);

    return true;
}

bool TestUtil::get_scene_test_files(std::vector<std::filesystem::path>& outFiles)
{
    fs::path dirPath;
    if (!get_scene_test_dir_path(&dirPath))
        return false;

    dirPath = dirPath / "Suite";
    if (!fs::is_directory(dirPath))
        return false;

    for (const auto& entry : fs::directory_iterator(dirPath))
    {
        if (entry.path().extension() == ".lua")
            outFiles.push_back(entry.path());
    }

    return true;
}

bool TestUtil::get_scene_test_script_files(std::vector<std::filesystem::path>& outFiles)
{
    fs::path dirPath;
    if (!get_scene_test_dir_path(&dirPath))
        return false;

    dirPath = dirPath / "Scripts";
    if (!fs::is_directory(dirPath))
        return false;

    for (const auto& entry : fs::directory_iterator(dirPath))
    {
        if (entry.path().extension() == ".lua")
            outFiles.push_back(entry.path());
    }

    return true;
}

bool TestUtil::get_root_directory_path(std::filesystem::path* path)
{
    const char* candidates[] = {
        "../Ludens/README.md",
        "../../Ludens/README.md",
        "../../../Ludens/README.md",
        "../../../../Ludens/README.md",
        "../../../../../Ludens/README.md",
    };

    for (const char* candidate : candidates)
    {
        if (fs::exists(candidate))
        {
            fs::path rootDirectory(candidate);
            if (path)
                *path = rootDirectory.parent_path();
            return true;
        }
    }

    return false;
}

bool TestUtil::get_test_dir_path(std::filesystem::path& path)
{
    path.clear();

    try
    {
        path = fs::temp_directory_path();
    }
    catch (const fs::filesystem_error& e)
    {
        return false;
    }

    path /= "Ludens";

    return true;
}

bool TestUtil::get_runtime_path(std::filesystem::path& path)
{
    if (!get_root_directory_path(&path))
        return false;

    // Kinda flaky, this is assuming MSVC on Windows and ninja on Linux.
    // Will modify this as necessary over time.
    const char* candidates[] = {
#ifdef LD_PLATFORM_WIN32
        "Build/LDRuntime/RuntimeMain/Release/LDRuntime.exe",
        "Build/LDRuntime/RuntimeMain/Debug/LDRuntime.exe",
#else
        "BuildLinux/LDRuntime/RuntimeMain/LDRuntime",
        "buildLinux/LDRuntime/RuntimeMain/LDRuntime",
        "Build/LDRuntime/RuntimeMain/LDRuntime",
        "build/LDRuntime/RuntimeMain/LDRuntime",
#endif
    };

    for (const char* candidate : candidates)
    {
        fs::path runtimePath = fs::canonical(path / candidate);

        if (fs::is_regular_file(runtimePath))
        {
            path = runtimePath;
            return true;
        }
    }

    return false;
}

} // namespace LD
