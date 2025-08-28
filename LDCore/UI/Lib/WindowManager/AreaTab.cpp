#include "AreaTab.h"
#include "UIWindowManagerObj.h"
#include <Ludens/Header/Assert.h>
#include <Ludens/System/Memory.h>

namespace LD {

void AreaTabControl::startup(UIContext ctx)
{
    mActiveTab = nullptr;

    UILayoutInfo layoutI{};
    layoutI.childAxis = UIAxis::UI_AXIS_X;
    layoutI.childGap = 0.0f;
    layoutI.childPadding = {.left = 10.0f, .right = 10.0f};
    layoutI.sizeX = UISize::fit();
    layoutI.sizeY = UISize::fixed(WINDOW_TAB_HEIGHT);

    UIWindowInfo windowI{};
    windowI.name = "windowTab";
    windowI.defaultMouseControls = false;
    mWindow = ctx.add_window(layoutI, windowI, this);
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
    UITheme theme = mWindow.get_theme();
    Rect windowRect = mWindow.get_rect();
    renderer.draw_rect(windowRect, theme.get_surface_color());

    for (AreaTab* tab : mTabs)
        tab->draw(renderer);

    if (mActiveTab)
    {
        UIWindow client = mActiveTab->client;
        client.on_draw(renderer);
    }
}

void AreaTabControl::invalidate_area(const Rect& area)
{
    mWindow.set_pos(area.get_pos());

    Rect clientArea(area.x, area.y + WINDOW_TAB_HEIGHT, area.w, area.h - WINDOW_TAB_HEIGHT);

    for (AreaTab* tab : mTabs)
    {
        tab->client.set_rect(clientArea);

        if (tab->onWindowResize)
            tab->onWindowResize(tab->client, clientArea.get_size());
    }
}

AreaTab::AreaTab(UIWindow client, UIWindow tabControl)
    : client(client), onWindowResize(nullptr)
{
    UITextWidgetInfo textWI{};
    textWI.cstr = nullptr;
    textWI.fontSize = WINDOW_TAB_HEIGHT * 0.7f; // TODO:
    textWI.hoverHL = false;
    titleText = tabControl.node().add_text({}, textWI, this);
}

void AreaTab::draw(ScreenRenderComponent renderer)
{
    titleText.on_draw(renderer);
}

} // namespace LD