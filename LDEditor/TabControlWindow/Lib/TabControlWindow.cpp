#include <Ludens/Header/Impulse.h>
#include <Ludens/UI/UIImmediate.h>
#include <LudensEditor/EditorContext/EditorIconAtlas.h>
#include <LudensEditor/TabControlWindow/TabControlWindow.h>

#include <format>
#include <string>

namespace LD {

struct TabControlWindowObj : EditorWindowObj
{
    std::string tabName;
    EditorWindowType tabType = EDITOR_WINDOW_TYPE_ENUM_COUNT;
    EditorIcon tabIcon = EDITOR_ICON_ENUM_LAST;
    Impulse dragImpulse = {};
    MouseButton dragBtn = {};
    Vec2 dragPos = {};
    bool dragBegin = false;

    TabControlWindowObj(const EditorWindowInfo& info)
        : EditorWindowObj(info) {}

    inline void set_tab_name(const char* name)
    {
        if (name)
            tabName = std::string(name);
        else
            tabName.clear();
    }

    void tab();
    void update();
};

void TabControlWindowObj::tab()
{
    UITheme uiTheme = theme.get_ui_theme();
    UIPanelStorage* panelS;
    UIImageStorage* imageS;

    const float height = theme.get_text_row_height();

    UILayoutInfo layoutI{};
    layoutI.sizeY = UISize::grow();
    layoutI.sizeX = UISize::fit();
    layoutI.childAxis = UI_AXIS_X;
    layoutI.childPadding.left = 6.0f;
    layoutI.childPadding.right = 6.0f;
    layoutI.childGap = 6.0f;
    panelS = ui_push_panel(nullptr);
    panelS->color = theme.get_ui_theme().get_surface_color();
    ui_top_layout(layoutI);
    if (tabIcon != EDITOR_ICON_ENUM_LAST)
    {
        MouseButton btn;
        Rect iconRect = EditorIconAtlas::get_icon_rect(tabIcon);
        float iconSize = height * 0.9f;
        imageS = ui_push_image(nullptr, iconSize, iconSize);
        imageS->image = ctx.get_editor_icon_atlas();
        imageS->rect = iconRect;
        ui_pop();
    }
    ui_push_text(nullptr, tabName.c_str());
    ui_pop();
    ui_pop();
}

void TabControlWindowObj::update()
{
    MouseButton btn;
    Vec2 pos;
    bool begin;
    Color tabBGColor;
    theme.get_tab_background_color(tabBGColor);

    begin_update_window();

    ui_window_set_color(tabBGColor);
    ui_top_layout_child_axis(UI_AXIS_X);
    if (ui_top_drag(btn, pos, begin))
    {
        dragImpulse.set(true);
        dragBtn = btn;
        dragPos = pos;
        dragBegin = begin;
    }

    tab();

    end_update_window();
}

//
// Public API
//

EditorWindow TabControlWindow::create(const EditorWindowInfo& windowI)
{
    auto* obj = heap_new<TabControlWindowObj>(MEMORY_USAGE_UI, windowI);

    return EditorWindow((EditorWindowObj*)obj);
}

void TabControlWindow::destroy(EditorWindow window)
{
    auto* obj = static_cast<TabControlWindowObj*>(window.unwrap());

    heap_delete<TabControlWindowObj>(obj);
}

void TabControlWindow::update(EditorWindowObj* base, const EditorUpdateTick& tick)
{
    auto* obj = (TabControlWindowObj*)base;

    (void)tick;
    obj->update();
}

void TabControlWindow::set_window_type(EditorWindowType type, const char* name, EditorIcon icon)
{
    mObj->tabType = type;
    mObj->tabIcon = icon;
    mObj->set_tab_name(name);
}

void TabControlWindow::set_window_name(const char* name)
{
    mObj->set_tab_name(name);
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