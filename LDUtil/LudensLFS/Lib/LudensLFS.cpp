#include <LDUtil/LudensLFS/Include/LudensLFS.h>
#include <cassert>
#include <iostream>

namespace fs = std::filesystem;

namespace LD {

LudensLFS::LudensLFS()
{
    const char* candidates[] = {
        "../../../Extra/LudensLFS/README.md",
        "../../../../Ludens/Extra/LudensLFS/README.md",
        "../../../../../Ludens/Extra/LudensLFS/README.md",
    };

    for (const char* candidate : candidates)
    {
        if (fs::exists(candidate))
        {
            fs::path lfsDirectory(candidate);
            lfsPath = lfsDirectory.parent_path();
            break;
        }
    }

    if (lfsPath.empty())
    {
        std::cout << "Failed to locate LudensLFS git submodule" << std::endl;
        return;
    }

    materialIconsPath = lfsPath / fs::path("Tmp/google/material_icons.png");
    assert(fs::exists(materialIconsPath));

    fontPath = lfsPath / fs::path("Fonts/Inter_24pt-Regular.ttf");
    assert(fs::exists(fontPath));

    skyboxFolderPath = lfsPath / fs::path("Tmp/skybox/learnopengl");
    assert(fs::exists(skyboxFolderPath));

    projectPath = lfsPath / fs::path("Tmp/projects/project1/project.toml");
    assert(fs::exists(projectPath));

    std::cout << "LudensLFS git submodule located at: " << lfsPath << std::endl;
}

/// @brief Locates the LudensLFS submodule during CRT initialization.
LudensLFS sLudensLFS;

} // namespace LD