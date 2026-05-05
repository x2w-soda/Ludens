#include <LudensUtil/TestUtil/TestUtil.h>

namespace fs = std::filesystem;

namespace LD {

TestUtil::TestUtil()
{
    if (!get_root_directory_path(&rootDirPath))
        rootDirPath.clear();
}

bool TestUtil::get_scene_test_directory_path(std::filesystem::path* path)
{
    fs::path rootPath;
    if (!get_root_directory_path(&rootPath))
        return false;

    fs::path dirPath = rootPath / "LDCore/Scene/Test/Suite";
    if (!fs::is_directory(dirPath))
        return false;

    if (path)
        *path = fs::absolute(dirPath);

    return true;
}

bool TestUtil::get_scene_test_files(std::vector<std::filesystem::path>& outFiles)
{
    fs::path dirPath;
    if (!get_scene_test_directory_path(&dirPath))
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

} // namespace LD