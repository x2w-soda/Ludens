#pragma once

#include <filesystem>

namespace LD {

/// @brief Ludens LFS is a git submodule located at https://github.com/x2w-soda/LudensLFS.
///        It contains temporary, intermediate, and binary files that are tracked by Git-LFS.
struct LudensLFS
{
    /// @brief Attempts to locate resources in Ludens LFS submodule.
    LudensLFS();

    /// @brief Statically locate LFS directory path.
    /// @note Mostly a static helper for doctest to skip test cases during CRT initialization.
    static bool get_directory_path(std::filesystem::path* path = nullptr);

    bool isFound;

    std::filesystem::path lfsPath;
    std::filesystem::path materialIconsPath;
    std::filesystem::path fontPath;
    std::filesystem::path skyboxFolderPath;
    std::filesystem::path projectPath;

    struct Audio
    {
        std::filesystem::path forestAmbiencePath;
        std::filesystem::path uiClick1Path;
        std::filesystem::path uiClick2Path;
    } audio;

    struct Test
    {
        std::filesystem::path emptyFilePath;
        std::filesystem::path nonExistentFilePath;
    } test;
};

/// @brief If the LDLudensLFS library is linked, this attempts to
///        locate resources during CRT initialization.
extern LudensLFS sLudensLFS;

} // namespace LD