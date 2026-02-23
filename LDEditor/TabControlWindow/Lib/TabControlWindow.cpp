#include <Ludens/Header/Impulse.h>
#include <Ludens/UI/UIImmediate.h>
#include <LudensEditor/EditorContext/EditorIconAtlas.h>
#include <LudensEditor/TabControlWindow/TabControlWindow.h>

#include <format>
#include <string>

namespace LD {

struct TabControlWindowObj : EditorWindowObj
{
    EditorContext ctx;
    UIWorkspace space;
    UIWindow root;
    EditorWindowType type;
    std::string tabName;
    EditorIcon tabIcon = EDITOR_ICON_ENUM_LAST;
    Impulse dragImpulse;
    MouseButton dragBtn;
    Vec2 dragPos;
    bool dragBegin;

    virtual inline EditorWindowType get_type() override { return EDITOR_WINDOW_TAB_CONTROL; }
    virtual void on_imgui(float delta) override;
};

void TabControlWindowObj::on_imgui(float delta)
{
    EditorTheme edTheme = ctx.get_theme();
    UITheme uiTheme = edTheme.get_ui_theme();
    Color surfaceColor = uiTheme.get_surface_color();
    Color tabBGColor;
    edTheme.get_tab_background_color(tabBGColor);

    MouseButton btn;
    Vec2 pos;
    bool begin;

    root.set_color(tabBGColor);
    ui_push_window(root);
    if (ui_top_drag(btn, pos, begin))
    {
        dragImpulse.set(true);
        dragBtn = btn;
        dragPos = pos;
        dragBegin = begin;
    }

    UILayoutInfo layoutI{};
    layoutI.sizeY = UISize::grow();
    layoutI.sizeX = UISize::fit();
    layoutI.childAxis = UI_AXIS_X;
    layoutI.childPadding.left = 6.0f;
    layoutI.childPadding.right = 6.0f;
    layoutI.childGap = 6.0f;
    ui_push_panel();
    ui_panel_color(surfaceColor);
    ui_top_layout(layoutI);
    if (tabIcon != EDITOR_ICON_ENUM_LAST)
    {
        MouseButton btn;
        Rect iconRect = EditorIconAtlas::get_icon_rect(tabIcon);
        RImage image = ctx.get_editor_icon_atlas();
        float iconSize = edTheme.get_font_size() * 1.2f;
        ui_push_image(image, iconSize, iconSize, 0xFFFFFFFF, &iconRect);
        ui_pop();
    }
    ui_push_text(tabName.c_str());
    ui_pop();
    ui_pop();
    ui_pop_window();
}

//
// Public API
//

EditorWindow TabControlWindow::create(const EditorWindowInfo& windowI)
{
    auto* obj = heap_new<TabControlWindowObj>(MEMORY_USAGE_UI);
    obj->ctx = windowI.ctx;
    obj->space = windowI.space;

    UILayoutInfo layoutI{};
    layoutI.childAxis = UI_AXIS_X;
    obj->root = obj->space.create_window(obj->space.get_root_id(), layoutI, {}, nullptr);

    return EditorWindow((EditorWindowObj*)obj);
}

void TabControlWindow::destroy(EditorWindow window)
{
    LD_ASSERT(window && window.get_type() == EDITOR_WINDOW_TAB_CONTROL);
    auto* obj = static_cast<TabControlWindowObj*>(window.unwrap());

    obj->space.destroy_window(obj->root);

    heap_delete<TabControlWindowObj>(obj);
}

void TabControlWindow::set_window_type(EditorWindowType type, const char* name, EditorIcon icon)
{
    mObj->type = type;
    mObj->tabIcon = icon;

    if (name)
        mObj->tabName = std::string(name);
    else
        mObj->tabName.clear();
}

bool TabControlWindow::has_drag(MouseButton& dragBtn, Vec2& screenPos, bool& dragBegin)
{
    bool hasDrag = mObj->dragImpulse.read();

    if (hasDrag)
    {
        dragBtn = mObj->dragBtn;
        screenPos = mObj->dragPos;
        dragBegin = mObj->dragBegin;
    }

    return hasDrag;
}

} // namespace LD