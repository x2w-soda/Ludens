#pragma once

#include <Ludens/UI/UILayout.h>
#include <Ludens/UI/UIWorkspace.h>
#include <LudensEditor/EditorContext/EditorContext.h>

namespace LD {

enum EditorWindowType
{
    EDITOR_WINDOW_TAB_CONTROL,
    EDITOR_WINDOW_SELECTION,
    EDITOR_WINDOW_VIEWPORT,
    EDITOR_WINDOW_OUTLINER,
    EDITOR_WINDOW_INSPECTOR,
    EDITOR_WINDOW_CONSOLE,
    EDITOR_WINDOW_VERSION,
    EDITOR_WINDOW_TYPE_ENUM_COUNT,
};

/// @brief Base class for editor window implementations.
struct EditorWindowObj
{
    virtual ~EditorWindowObj() = default;

    /// @brief Type reflection for handle down-casting.
    virtual EditorWindowType get_type() = 0;

    /// @brief Derived class populates UIWindows with UIImmediate API,
    //         callers prepares ui_frame_begin / ui_frame_end scope.
    virtual void on_imgui(float delta) = 0;
};

struct EditorWindowInfo
{
    EditorContext ctx;
    UIWorkspace space;
};

struct EditorWindow : Handle<struct EditorWindowObj>
{
    inline EditorWindowType get_type() { return mObj->get_type(); }

    inline void on_imgui(float delta) { mObj->on_imgui(delta); }
};

} // namespace LD