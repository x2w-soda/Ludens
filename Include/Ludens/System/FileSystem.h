#pragma once

#include <Ludens/Header/Types.h>
#include <cstdint>
#include <filesystem>

namespace LD {
namespace FS {

using Path = std::filesystem::path;

uint64_t get_file_size(const Path& path);

bool read_file(const Path& path, uint64_t& size, byte* buf);

bool write_file(const Path& path, uint64_t size, const byte* buf);

bool exists(const Path& path);

} // namespace FS
} // namespace LD