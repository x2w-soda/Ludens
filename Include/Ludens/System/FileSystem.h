#pragma once

#include <Ludens/Header/Types.h>
#include <cstdint>
#include <filesystem>

namespace LD {
namespace FS {

uint64_t get_file_size(const std::filesystem::path& path);

bool read_file(const std::filesystem::path& path, uint64_t& size, byte* buf);

bool exists(const std::filesystem::path& path);

} // namespace FS
} // namespace LD