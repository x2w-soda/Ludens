#pragma once

#include <Ludens/Header/Types.h>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace LD {
namespace FS {

using Path = std::filesystem::path;

/// @brief Query the contents of a directory, non-recursively.
/// @param directory Path to directory to query.
/// @param contents Output contents of the directory.
/// @param err Output error message.
/// @return True on success.
bool get_directory_content(const Path& directory, std::vector<Path>& contents, std::string& err);

/// @brief Get file size in bytes.
uint64_t get_file_size(const Path& path);

/// @brief Read whole file into buffer.
bool read_file(const Path& path, uint64_t& size, byte* buf);

/// @brief Write bytes to a file.
bool write_file(const Path& path, uint64_t size, const byte* buf);

/// @brief Check if path exists in filesystem.
bool exists(const Path& path);

/// @brief Check if path exists and is a directory.
bool is_directory(const Path& path);

/// @brief Filter files using extensions.
/// @param paths Vector of paths, directories are not disturbed.
/// @param extension File extension to filter files.
void filter_files_by_extension(std::vector<FS::Path>& paths, const char* extension);

} // namespace FS
} // namespace LD