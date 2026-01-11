#include <Ludens/Header/Version.h>
#include <Ludens/UI/UIImmediate.h>
#include <LudensEditor/VersionWindow/VersionWindow.h>

#include <format>
#include <string>

namespace LD {

struct VersionWindowObj : EditorWindowObj
{
    EditorContext ctx;
    UIWorkspace space;
    UIWindow root;

    virtual inline EditorWindowType get_type() override { return EDITOR_WINDOW_VERSION; }
    virtual void on_imgui(float delta) override;
};

void VersionWindowObj::on_imgui(float delta)
{
    EditorTheme theme = ctx.get_theme();
    UITheme uiTheme = theme.get_ui_theme();

    root.set_color(uiTheme.get_surface_color());
    ui_push_window(root);

    std::string version = std::format("Version {}.{}.{}", LD_VERSION_MAJOR, LD_VERSION_MINOR, LD_VERSION_PATCH);
    ui_push_text(version.c_str());
    ui_pop();

#ifdef NDEBUG
    std::string build("Release Build");
#else
    std::string build("Debug Build");
#endif

    ui_push_text(build.c_str());
    ui_pop();

    ui_pop_window();
}

//
// Public API
//

EditorWindow VersionWindow::create(const EditorWindowInfo& windowI)
{
    VersionWindowObj* obj = heap_new<VersionWindowObj>(MEMORY_USAGE_UI);
    obj->ctx = windowI.ctx;
    obj->space = windowI.space;

    EditorTheme theme = obj->ctx.get_theme();
    float pad = theme.get_padding();

    UILayoutInfo layoutI{};
    layoutI.childAxis = UI_AXIS_Y;
    layoutI.childAlignY = UI_ALIGN_BEGIN;
    layoutI.childPadding.left = pad;
    layoutI.childPadding.right = pad;
    obj->root = obj->space.create_window(obj->space.get_root_id(), layoutI, {}, nullptr);
    obj->root.hide();

    return EditorWindow((EditorWindowObj*)obj);
}

void VersionWindow::destroy(EditorWindow window)
{
    LD_ASSERT(window && window.get_type() == EDITOR_WINDOW_VERSION);
    auto* obj = static_cast<VersionWindowObj*>(window.unwrap());

    obj->space.destroy_window(obj->root);

    heap_delete<VersionWindowObj>(obj);
}

void VersionWindow::show()
{
    auto* obj = (VersionWindowObj*)mObj;

    obj->root.show();
}

} // namespace LD