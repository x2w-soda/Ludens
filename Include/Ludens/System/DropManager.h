#pragma once

#include <Ludens/Header/Handle.h>
#include <Ludens/System/FileSystem.h>
#include <cstddef>

struct GLFWwindow;

namespace LD {

/// @brief Callback when user drags files into application window.
///        This function should not block, copy these paths and defer IO operations.
typedef void (*DropTargetFileCallback)(size_t fileCount, const FS::Path* files);

struct DropTarget : Handle<struct DropTargetObj>
{
    /// @brief Create drop target for window. Currently only Win32 supports
    ///        file drag and drop via OLE IDropTarget.
    static DropTarget create(GLFWwindow* window, DropTargetFileCallback onDropFile);

    /// @brief Destroy drop target associated with window.
    static void destroy(DropTarget target);
};

} // namespace LD