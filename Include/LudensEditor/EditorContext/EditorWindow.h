#pragma once

#include <Ludens/UI/UIImmediate.h>
#include <Ludens/UI/UILayout.h>
#include <LudensEditor/EditorContext/EditorContext.h>

#include <string>

namespace LD {

enum EditorWindowType
{
    EDITOR_WINDOW_TAB_CONTROL,
    EDITOR_WINDOW_SELECTION,
    EDITOR_WINDOW_CREATE_COMPONENT,
    EDITOR_WINDOW_PROJECT_SETTINGS,
    EDITOR_WINDOW_VIEWPORT,
    EDITOR_WINDOW_OUTLINER,
    EDITOR_WINDOW_INSPECTOR,
    EDITOR_WINDOW_CONSOLE,
    EDITOR_WINDOW_VERSION,
    EDITOR_WINDOW_TYPE_ENUM_COUNT,
};

struct EditorWindowInfo
{
    EditorContext ctx;
    const char* uiWorkspaceName;
};

/// @brief Base class for editor window implementations.
class EditorWindowObj
{
public:
    EditorWindowObj() = delete;
    EditorWindowObj(const EditorWindowInfo& info)
        : mUIWorkspaceName(info.uiWorkspaceName), mCtx(info.ctx) {}
    virtual ~EditorWindowObj() = default;

    /// @brief Type reflection for handle down-casting.
    virtual EditorWindowType get_type() = 0;

    /// @brief Derived class populates UIWindows with UIImmediate API,
    //         callers prepares ui_frame_begin / ui_frame_end scope.
    virtual void on_imgui(float delta) = 0;

    inline bool should_close() const { return mShouldClose; };
    inline void set_pos(const Vec2& pos) { mRootRect.set_pos(pos.x, pos.y); }
    inline void set_rect(const Rect& rect) { mRootRect = rect; }

    inline void ui_workspace_begin() { LD::ui_workspace_begin(mUIWorkspaceName.c_str(), mRootRect); }

protected:
    std::string mUIWorkspaceName;
    EditorContext mCtx;
    Rect mRootRect{};
    bool mShouldClose = false;
};

/// @brief Composes one UIWorkspace, belongs to the same UILayer as it's container EditorWorkspace.
struct EditorWindow : Handle<class EditorWindowObj>
{
    inline EditorWindowType get_type() { return mObj->get_type(); }

    inline void on_imgui(float delta) { mObj->on_imgui(delta); }

    /// @brief Hint at the EditorWorkspace that this window should be destroyed.
    inline bool should_close() const { return mObj->should_close(); };

    inline void set_pos(const Vec2& pos) { mObj->set_pos(pos); }
    inline void set_rect(const Rect& rect) { mObj->set_rect(rect); }
};

} // namespace LD