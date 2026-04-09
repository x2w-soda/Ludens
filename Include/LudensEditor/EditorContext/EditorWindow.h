#pragma once

#include <Ludens/UI/UIImmediate.h>
#include <Ludens/UI/UILayout.h>
#include <LudensEditor/EditorContext/EditorContext.h>

#include <string>

namespace LD {

enum EditorWindowType
{
    EDITOR_WINDOW_TAB_CONTROL,
    EDITOR_WINDOW_ASSET_IMPORT,
    EDITOR_WINDOW_ASSET_SELECT,
    EDITOR_WINDOW_SELECTION,
    EDITOR_WINDOW_CREATE_COMPONENT,
    EDITOR_WINDOW_PROJECT_SETTINGS,
    EDITOR_WINDOW_PROJECT,
    EDITOR_WINDOW_DOCUMENT,
    EDITOR_WINDOW_VIEWPORT,
    EDITOR_WINDOW_OUTLINER,
    EDITOR_WINDOW_INSPECTOR,
    EDITOR_WINDOW_CONSOLE,
    EDITOR_WINDOW_VERSION,
    EDITOR_WINDOW_TYPE_ENUM_COUNT,
};

struct EditorWindowInfo
{
    EditorWindowType type;
    EditorContext ctx;
    const char* name;
    const char* uiWorkspaceName;
};

/// @brief An EditorWindow corresponds to a UIWorkspace, and can therefore consist of multiple UIWindows.
struct EditorWindowObj
{
    EditorWindowType type = EDITOR_WINDOW_TYPE_ENUM_COUNT; // required for polymorphic dispatch
    std::string name;                                      // display name in tabs
    std::string uiWorkspaceName;                           // unique imgui workspace name
    EditorContext ctx = {};                                // editor context handle
    EditorTheme theme = {};                                // editor theme handle
    Rect rootRect = {};                                    // editor window rect in screen space
    bool shouldClose = false;                              // hint that the editor window should be closed

    EditorWindowObj(const EditorWindowInfo& info)
        : type(info.type), ctx(info.ctx), name(info.name), uiWorkspaceName(info.uiWorkspaceName)
    {
        theme = ctx.get_theme();
    }

    inline void begin_update_window()
    {
        ui_workspace_begin(uiWorkspaceName.c_str(), rootRect);
        ui_push_window(uiWorkspaceName.c_str());
    }

    inline void end_update_window()
    {
        ui_pop_window();
        ui_workspace_end();
    }
};

/// @brief Editor window public handle.
struct EditorWindow : Handle<struct EditorWindowObj>
{
    inline EditorWindowType type() { return mObj->type; }
    inline std::string get_name() { return mObj->name; }
    inline std::string get_ui_workspace_name() { return mObj->uiWorkspaceName; }

    /// @brief Hint at the EditorWorkspace that this window should be destroyed.
    inline bool should_close() const { return mObj->shouldClose; };

    inline void set_pos(Vec2 pos) { mObj->rootRect.set_pos(pos.x, pos.y); }
    inline void set_rect(Rect rect) { mObj->rootRect = rect; }
};

} // namespace LD
