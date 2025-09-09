#include "AreaTab.h"
#include "UIWindowManagerObj.h"
#include <Ludens/Header/Assert.h>
#include <Ludens/System/Memory.h>

namespace LD {

void AreaTabControl::startup_as_leaf(UIContext ctx, const Rect& area)
{
    mActiveTab = nullptr;
    mIsFloat = false;
    mFloatBorder = 0.0f;

    UILayoutInfo layoutI{};
    layoutI.childAxis = UIAxis::UI_AXIS_X;
    layoutI.sizeX = UISize::fixed(area.w);
    layoutI.sizeY = UISize::fixed(WINDOW_TAB_HEIGHT);

    UIWindowInfo windowI{};
    windowI.name = "AreaTabControl";
    windowI.defaultMouseControls = false;
    mWindow = ctx.add_window(layoutI, windowI, this);
    mWindow.set_pos(area.get_pos());
}

void AreaTabControl::startup_as_float(UIContext ctx, const Rect& area, float border)
{
    LD_ASSERT(area.w > 0.0f && area.h > 0.0f);

    mActiveTab = nullptr;
    mIsFloat = true;
    mFloatBorder = border;

    UILayoutInfo layoutI{};
    layoutI.childAxis = UIAxis::UI_AXIS_X;
    layoutI.childPadding = {.left = border, .right = border, .bottom = border};
    layoutI.sizeX = UISize::fixed(area.w);
    layoutI.sizeY = UISize::fixed(area.h);

    UIWindowInfo windowI{};
    windowI.name = "AreaTabControl";
    windowI.defaultMouseControls = false;
    mWindow = ctx.add_window(layoutI, windowI, this);
    mWindow.set_pos(area.get_pos());
    mWindow.set_on_drag(&AreaTabControl::on_float_drag);
    mWindow.set_on_draw(&AreaTabControl::on_float_draw);
}

void AreaTabControl::cleanup()
{
    for (AreaTab* tab : mTabs)
        heap_delete<AreaTab>(tab);

    mTabs.clear();
}

void AreaTabControl::add_tab(UIWindow client)
{
    AreaTab* tab = heap_new<AreaTab>(MEMORY_USAGE_UI, client, mWindow);
    mTabs.push_back(tab);
    mActiveTab = tab;
}

AreaTab* AreaTabControl::get_active_tab()
{
    LD_ASSERT(mActiveTab);

    return mActiveTab;
}

void AreaTabControl::draw(ScreenRenderComponent renderer)
{
    // draws all tabs
    mWindow.draw(renderer);

    if (mActiveTab)
    {
        UIWindow client = mActiveTab->client;
        client.draw(renderer);
    }
}

void AreaTabControl::invalidate_area(const Rect& area)
{
    if (mIsFloat)
        invalidate_float_area(area);
    else
        invalidate_leaf_area(area);
}

void AreaTabControl::invalidate_float_area(const Rect& area)
{
    mWindow.set_rect(area);

    // Subtract left, bottom, and right borders
    Rect clientArea(area.x + mFloatBorder, area.y + WINDOW_TAB_HEIGHT, area.w - 2 * mFloatBorder, area.h - mFloatBorder);

    for (AreaTab* tab : mTabs)
    {
        tab->client.set_rect(clientArea);

        if (tab->onClientResize)
            tab->onClientResize(tab->client, clientArea.get_size());
    }
}

void AreaTabControl::invalidate_leaf_area(const Rect& area)
{
    float areaW = area.w;
    mWindow.set_pos(area.get_pos());
    mWindow.set_size(Vec2(areaW, WINDOW_TAB_HEIGHT));

    Rect clientArea(area.x, area.y + WINDOW_TAB_HEIGHT, area.w, area.h - WINDOW_TAB_HEIGHT);

    for (AreaTab* tab : mTabs)
    {
        tab->client.set_rect(clientArea);

        if (tab->onClientResize)
            tab->onClientResize(tab->client, clientArea.get_size());
    }
}

void AreaTabControl::on_float_drag(UIWidget widget, MouseButton btn, const Vec2& dragPos, bool begin)
{
    // TODO:
}

void AreaTabControl::on_float_draw(UIWidget widget, ScreenRenderComponent renderer)
{
    UITheme theme = widget.get_theme();
    Rect windowRect = widget.get_rect();
    renderer.draw_rect(windowRect, theme.get_background_color());
}

AreaTab::AreaTab(UIWindow client, UIWindow tabControl)
    : client(client), onClientResize(nullptr)
{
    UINode tabControlNode = tabControl.node();
    UITheme theme = tabControl.get_theme();

    UILayoutInfo layoutI{};
    layoutI.childAxis = UI_AXIS_X;
    layoutI.childPadding = {.left = 10.0f, .right = 10.0f};
    layoutI.sizeX = UISize::fit();
    layoutI.sizeY = UISize::fixed(WINDOW_TAB_HEIGHT);
    UIPanelWidgetInfo panelWI{};
    panelWI.color = theme.get_surface_color();
    panelW = tabControlNode.add_panel(layoutI, panelWI, this);

    std::string clientName = client.get_name();
    UITextWidgetInfo textWI{};
    textWI.cstr = clientName.c_str();
    textWI.fontSize = WINDOW_TAB_HEIGHT * 0.7f; // TODO:
    textWI.hoverHL = false;
    titleTextW = panelW.node().add_text({}, textWI, this);
}

} // namespace LD