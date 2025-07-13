#pragma once

#include <vector>
#include <string>

namespace LD {

/// @brief find test executables in a directory recursively.
/// @param directory search directory
/// @param paths output executable paths
/// @param extension filter with file extensions
void find_test_executables(const char* directory, std::vector<std::string>& paths, const char* extension);

/// @brief run test executables
/// @param paths paths to executables
/// @return number of processes that return 0
/// @warning we will be running executables with std::system, the builder
///          should not have root/admin privileges and we have no way to
///          check whether the exectables are malicious.
int run_test_exectuables(std::vector<std::string>& paths);

} // namespace LD