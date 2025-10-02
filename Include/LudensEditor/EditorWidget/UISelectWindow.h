#pragma once

#include <Ludens/Header/Handle.h>
#include <Ludens/System/FileSystem.h>
#include <Ludens/UI/UIContext.h>
#include <LudensEditor/EditorContext/EditorContext.h>
#include <LudensEditor/EditorContext/EditorSettings.h>

namespace LD {

struct UISelectWindowInfo
{
    UIContext context;
    EditorContext editorCtx;
    FS::Path directory;
};

/// @brief Window for item selection in physical filesystem.
struct UISelectWindow : Handle<struct UISelectWindowObj>
{
    /// @brief Create window for file selection.
    static UISelectWindow create(const UISelectWindowInfo& info);

    /// @brief Destroy window.
    static void destroy(UISelectWindow window);

    /// @brief Get UI framework window handle.
    UIWindow get_handle();

    /// @brief Set directory to display.
    void set_directory(const FS::Path& directory);

    /// @brief Filter files by extension.
    /// @param extension File extension not including '.', or nullptr to disable filter.
    void set_extension_filter(const char* extension);

    /// @brief Set callback when item is selected.
    void set_on_select(void (*onSelect)(const FS::Path& path, void* user), void* user);

    /// @brief Set callback when selection is canceled.
    void set_on_cancel(void (*onCancel)(void* user));
};

} // namespace LD