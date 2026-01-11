#pragma once

#include <Ludens/Header/Handle.h>
#include <Ludens/Header/Math/Rect.h>
#include <Ludens/UI/UILayer.h>
#include <LudensEditor/EditorContext/EditorWindow.h>

#include <cstdint>

namespace LD {

using EditorAreaID = uint32_t;

struct EditorWorkspaceInfo
{
    EditorContext ctx;
    UILayer layer;         // the actual UI layer this editor workspace belongs to
    Rect rootRect;         // total rect area for the editor workspace
    bool isVisible = true; // initial visibility state after creation
    bool isFloat = false;
};

/// @brief A container for one or more EditorWindow, belongs to a UILayer.
struct EditorWorkspace : Handle<struct EditorWorkspaceObj>
{
    /// @brief Create editor workspace.
    static EditorWorkspace create(const EditorWorkspaceInfo& spaceI);

    /// @brief Destroy editor workspace.
    static void destroy(EditorWorkspace space);

    /// @brief If hidden, skips all rendering for EditorWindows in this workspace.
    void set_visible(bool isVisible);

    /// @brief Create and add an editor window to the workspace.
    EditorWindow create_window(EditorAreaID areaID, EditorWindowType type);

    /// @brief Destroy an editor window in the workspace.
    /// @note Deferred destruction until next context update.
    void destroy_window(EditorWindow window);

    /// @brief Imgui pass on all EditorWindows in this workspace.
    void on_imgui(float delta);

    /// @brief Set editor workspace rect, triggers resize callbacks for EditorWindows.
    void set_rect(const Rect& area);

    /// @brief Get area ID of root.
    EditorAreaID get_root_id();

    /// @brief Split an area to make room for right.
    /// @return New area from right partition.
    EditorAreaID split_right(EditorAreaID areaID, float ratio);

    /// @brief Split an area to make room for bottom.
    /// @return New area from bottom partition.
    EditorAreaID split_bottom(EditorAreaID areaID, float ratio);
};

} // namespace LD