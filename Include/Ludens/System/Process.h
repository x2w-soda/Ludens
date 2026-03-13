#pragma once

#include <Ludens/System/FileSystem.h>

namespace LD {

/// @brief Opens a file using the system's default application.
/// @param path Path to file on disk.
/// @warning SECURITY: Input path is passed to Win32 ShellExecuteA() or Unix system(),
/// 	     ensure input originates from a trusted source.
void shell_open(const FS::Path& path);

} // namespace LD