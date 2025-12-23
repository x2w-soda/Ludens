#pragma once

#include <Ludens/Header/Platform.h>
#ifdef LD_PLATFORM_WIN32

#include <Ludens/Header/Handle.h>
#include <Ludens/System/FileSystem.h>
#include <string>

namespace LD {

/// @brief Win32 specific utilities.
struct Win32Util : Handle<struct Win32UtilObj>
{
    static Win32Util create();
    static void destroy(Win32Util util);

    /// @brief Try patch the icon of a PE file (.exe, .dll) by updating RT_ICON and RT_GROUP_ICON resources.
    /// @param path Relative path to a PE file on disk.
    /// @param icoPath Relative path to an .ico file on disk.
    /// @return True on success.
    /// @note This resource patch only reflects for the PE file in taskbar or console windows. A running HWND window
    ///       still need to use something like glfwSetWindowIcon (_glfwSetWindowIconWin32) to apply changes at runtime.
    bool patch_icon_resources(const FS::Path& path, const FS::Path& icoPath);
};

} // namespace LD
#endif // LD_PLATFORM_WIN32