#include "AreaTab.h"
#include "UIWindowManagerObj.h"
#include <Ludens/Header/Assert.h>
#include <Ludens/System/Memory.h>
#include <unordered_set>

namespace LD {

void AreaTabControl::startup_as_leaf(UIContext ctx, const Rect& area)
{
    mCtx = ctx;
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
    mWindow.set_on_update(&AreaTabControl::on_update);
}

void AreaTabControl::startup_as_float(UIContext ctx, const Rect& area, float border)
{
    LD_ASSERT(area.w > 0.0f && area.h > 0.0f);

    mCtx = ctx;
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
    mWindow.set_on_update(&AreaTabControl::on_update);
}

void AreaTabControl::cleanup()
{
    for (AreaTab* tab : mTabs)
        heap_delete<AreaTab>(tab);

    mTabs.clear();
    mCtx.remove_window(mWindow);
}

void AreaTabControl::add_tab(UIWindow client, void* user)
{
    AreaTab* tab = heap_new<AreaTab>(MEMORY_USAGE_UI, client, mWindow, user);
    mTabs.push_back(tab);
    mActiveTab = tab;
}

AreaTab* AreaTabControl::get_active_tab()
{
    LD_ASSERT(mActiveTab);

    return mActiveTab;
}

void AreaTabControl::show()
{
    mWindow.show();

    if (mActiveTab && mActiveTab->client)
        mActiveTab->client.show();
}

void AreaTabControl::hide()
{
    mWindow.hide();

    for (AreaTab* tab : mTabs)
    {
        if (tab->client)
            tab->client.hide();
    }
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
            tab->onClientResize(tab->client, clientArea.get_size(), tab->user);
    }
}

void AreaTabControl::invalidate_leaf_area(const Rect& area)
{
    float areaW = area.w;
    mWindow.set_pos(area.get_pos());
    mWindow.set_size(Vec2(areaW, WINDOW_TAB_HEIGHT));

    Rect clientArea = get_float_client_area(area);

    for (AreaTab* tab : mTabs)
    {
        tab->client.set_rect(clientArea);

        if (tab->onClientResize)
            tab->onClientResize(tab->client, clientArea.get_size(), tab->user);
    }
}

Rect AreaTabControl::get_float_client_area(const Rect& nodeArea)
{
    return Rect(nodeArea.x, nodeArea.y + WINDOW_TAB_HEIGHT, nodeArea.w, nodeArea.h - WINDOW_TAB_HEIGHT);
}

void AreaTabControl::on_update(UIWidget widget, float delta)
{
    AreaTabControl& self = *(AreaTabControl*)widget.get_user();

    std::unordered_set<AreaTab*> toErase;

    for (AreaTab* tab : self.mTabs)
    {
        if (tab->shouldClose)
        {
            if (tab == self.mActiveTab)
                self.mActiveTab = nullptr;

            toErase.insert(tab);
            heap_delete<AreaTab>(tab);
            break;
        }
    }

    std::erase_if(self.mTabs, [&](AreaTab* tab) { return toErase.contains(tab); });

    if (!self.mTabs.empty() && self.mActiveTab == nullptr)
        self.mActiveTab = self.mTabs.front();
}

void AreaTabControl::on_float_drag(UIWidget widget, MouseButton btn, const Vec2& dragPos, bool begin)
{
    // TODO:
}

void AreaTabControl::on_float_draw(UIWidget widget, ScreenRenderComponent renderer)
{
    AreaTabControl& self = *(AreaTabControl*)widget.get_user();
    UITheme theme = widget.get_theme();
    Rect rect = widget.get_rect(); // floating node area
    renderer.draw_rect(rect, theme.get_background_color());

    rect = self.get_float_client_area(rect);
    renderer.draw_rect(rect, theme.get_surface_color());
}

AreaTab::AreaTab(UIWindow client, UIWindow tabControl, void* user)
    : client(client), onClientResize(nullptr), onClientClose(nullptr), shouldClose(false), user(user)
{
    UINode tabControlNode = tabControl.node();
    UITheme theme = tabControl.get_theme();

    UILayoutInfo layoutI{};
    layoutI.childAxis = UI_AXIS_X;
    layoutI.childPadding = {.left = 10.0f, .right = 10.0f};
    layoutI.childGap = 15.0f;
    layoutI.sizeX = UISize::fit();
    layoutI.sizeY = UISize::fixed(WINDOW_TAB_HEIGHT);
    UIPanelWidgetInfo panelWI{};
    panelWI.color = theme.get_surface_color();
    rootW = tabControlNode.add_panel(layoutI, panelWI, this);

    std::string clientName = client.get_name();
    UITextWidgetInfo textWI{};
    textWI.cstr = clientName.c_str();
    textWI.fontSize = WINDOW_TAB_HEIGHT * 0.7f; // TODO:
    textWI.hoverHL = false;
    titleTextW = rootW.node().add_text({}, textWI, this);

    // TODO: Editor Icons
    textWI.cstr = "X";
    textWI.hoverHL = true;
    closeW = rootW.node().add_text({}, textWI, this);
    closeW.set_on_mouse(&AreaTab::on_close);
}

void AreaTab::on_close(UIWidget widget, const Vec2& pos, MouseButton btn, UIEvent event)
{
    if (event != UI_MOUSE_DOWN || btn != MOUSE_BUTTON_LEFT)
        return;

    AreaTab& self = *(AreaTab*)widget.get_user();

    // the window manager will no longer be interacting with client window
    if (self.onClientClose)
        self.onClientClose(self.client, self.user);

    // see AreaTabControl::on_update
    self.shouldClose = true;
}

} // namespace LD