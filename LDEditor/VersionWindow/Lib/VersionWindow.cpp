#include <Ludens/Header/Version.h>
#include <Ludens/UI/UIImmediate.h>
#include <LudensEditor/VersionWindow/VersionWindow.h>

#include <format>
#include <string>

namespace LD {

struct VersionWindowObj : EditorWindowObj
{
    VersionWindowObj(const EditorWindowInfo& info)
        : EditorWindowObj(info) {}

    void update(float delta);
};

void VersionWindowObj::update(float delta)
{
    EditorTheme theme = ctx.get_theme();
    UITheme uiTheme = theme.get_ui_theme();
    float pad = theme.get_child_pad();

    begin_update_window();

    ui_window_set_color(uiTheme.get_surface_color());

    UILayoutInfo layoutI{};
    layoutI.childAxis = UI_AXIS_Y;
    layoutI.childAlignY = UI_ALIGN_BEGIN;
    layoutI.childPadding.left = pad;
    layoutI.childPadding.right = pad;
    ui_top_layout(layoutI);

    std::string version = std::format("Version {}.{}.{}", LD_VERSION_MAJOR, LD_VERSION_MINOR, LD_VERSION_PATCH);
    ui_push_text(nullptr, version.c_str());
    ui_pop();

#ifdef NDEBUG
    std::string build("Release Build");
#else
    std::string build("Debug Build");
#endif

    ui_push_text(nullptr, build.c_str());
    ui_pop();

    end_update_window();
}

//
// Public API
//

EditorWindow VersionWindow::create(const EditorWindowInfo& windowI)
{
    VersionWindowObj* obj = heap_new<VersionWindowObj>(MEMORY_USAGE_UI, windowI);

    return EditorWindow((EditorWindowObj*)obj);
}

void VersionWindow::destroy(EditorWindow window)
{
    auto* obj = static_cast<VersionWindowObj*>(window.unwrap());

    heap_delete<VersionWindowObj>(obj);
}

void VersionWindow::update(EditorWindowObj* base, const EditorUpdateTick& tick)
{
    auto* obj = (VersionWindowObj*)base;

    obj->update(tick.delta);
}

} // namespace LD