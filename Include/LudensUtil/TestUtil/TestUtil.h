#pragma once

#include <filesystem>
#include <vector>

namespace LD {

struct TestUtil
{
    TestUtil();

    /// @brief Statically locate SceneTest directory path.
    /// @note Mostly a static helper for doctest to skip test cases during CRT initialization.
    static bool get_scene_test_directory_path(std::filesystem::path* path = nullptr);
    static bool get_scene_test_files(std::vector<std::filesystem::path>& outFiles);

    /// @brief Statically locate root directory path.
    /// @note Mostly a static helper for doctest to skip test cases during CRT initialization.
    static bool get_root_directory_path(std::filesystem::path* path = nullptr);


    std::filesystem::path rootDirPath = {};
};

/// @brief If the LDTestUtil library is linked, this attempts to
///        locate resources during CRT initialization.
extern TestUtil sTestUtil;

} // namespace LD