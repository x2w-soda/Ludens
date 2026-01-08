#pragma once

#include <Ludens/Header/Types.h>
#include <Ludens/Header/View.h>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace LD {
namespace FS {

using Path = std::filesystem::path;

Path current_path();

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

/// @brief Read whole file into byte vector.
bool read_file_to_vector(const FS::Path& path, std::vector<byte>& v);

/// @brief Write bytes to a file.
bool write_file(const Path& path, uint64_t size, const byte* buf, std::string& err);

bool write_file(const Path& path, const View& view, std::string& err);

/// @brief A safer write_file protocol, behaves the same as write_file if the save file does not exist.
///        Otherwise, the existing file is renamed as backup, the new contents are written to a tmp file,
///        and finally the tmp file is renamed to the save file path.
/// @return True if all steps of the protocol succeeded.
bool write_file_and_swap_backup(const Path& path, uint64_t size, const byte* buf, std::string& err);

bool write_file_and_swap_backup(const Path& path, const View& view, std::string& err);

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