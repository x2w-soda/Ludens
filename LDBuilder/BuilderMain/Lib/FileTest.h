#pragma once

#include <Ludens/System/FileSystem.h>

namespace LD {

/// @brief Utility similar to the linux 'file' command.
///        Checks file contents and extracts basic metadata.
struct FileTest
{
    static void check_file(const FS::Path& filePath);
};

} // namespace LD