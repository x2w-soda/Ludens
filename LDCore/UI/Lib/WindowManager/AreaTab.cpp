#include "AreaTab.h"
#include "UIWindowManagerObj.h"
#include <Ludens/Application/Application.h>
#include <Ludens/Header/Assert.h>
#include <Ludens/System/Memory.h>
#include <unordered_set>

#define OPACITY_ANIM_DURATION 0.1f

namespace LD {

void AreaTabControl::startup_as_leaf(UIWindowManagerObj* wm, const Rect& area)
{
    mWM = wm;
    mCtx = wm->get_context();
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
    windowI.layer = wm->get_ground_layer_hash();
    mWindow = mCtx.add_window(layoutI, windowI, this);
    mWindow.set_pos(area.get_pos());
    mWindow.set_on_update(&AreaTabControl::on_update);
}

void AreaTabControl::startup_as_float(UIWindowManagerObj* wm, const Rect& area, float border)
{
    LD_ASSERT(area.w > 0.0f && area.h > 0.0f);

    mWM = wm;
    mCtx = wm->get_context();
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
    windowI.hidden = true;
    windowI.layer = wm->get_float_layer_hash();
    mWindow = mCtx.add_window(layoutI, windowI, this);
    mWindow.set_pos(area.get_pos());
    mWindow.set_on_drag(&AreaTabControl::on_float_drag);
    mWindow.set_on_draw(&AreaTabControl::on_float_draw);
    mWindow.set_on_hover(&AreaTabControl::on_float_hover);
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
    AreaTab* tab = heap_new<AreaTab>(MEMORY_USAGE_UI, mWM, client, mWindow, user);
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
    mWindow.raise();
    mWindow.show();

    mOpacityA.showing(OPACITY_ANIM_DURATION);

    if (mActiveTab && mActiveTab->client)
    {
        mActiveTab->client.raise();
        mActiveTab->client.show();
    }
}

void AreaTabControl::hide()
{
    mOpacityA.hiding(OPACITY_ANIM_DURATION);
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

    Rect clientArea = get_float_client_area(area);

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

    Rect clientArea = get_leaf_client_area(area);

    invalidate_client_area(clientArea);
}

void AreaTabControl::invalidate_client_pos(const Vec2& pos)
{
    for (AreaTab* tab : mTabs)
    {
        tab->client.set_pos(pos);
    }
}

void AreaTabControl::invalidate_client_size(const Vec2& size)
{
    for (AreaTab* tab : mTabs)
    {
        tab->client.set_size(size);

        if (tab->onClientResize)
            tab->onClientResize(tab->client, size, tab->user);
    }
}

void AreaTabControl::invalidate_client_area(const Rect& area)
{
    for (AreaTab* tab : mTabs)
    {
        tab->client.set_rect(area);

        if (tab->onClientResize)
            tab->onClientResize(tab->client, area.get_size(), tab->user);
    }
}

Rect AreaTabControl::get_leaf_client_area(const Rect& nodeArea)
{
    return Rect(nodeArea.x, nodeArea.y + WINDOW_TAB_HEIGHT, nodeArea.w, nodeArea.h - WINDOW_TAB_HEIGHT);
}

Rect AreaTabControl::get_float_client_area(const Rect& nodeArea)
{
    return Rect(nodeArea.x + mFloatBorder,
                nodeArea.y + WINDOW_TAB_HEIGHT,
                nodeArea.w - 2 * mFloatBorder,
                nodeArea.h - WINDOW_TAB_HEIGHT - mFloatBorder);
}

void AreaTabControl::delete_tabs()
{
    if (mOpacityA.is_hiding())
        return;

    std::unordered_set<AreaTab*> toErase;

    size_t eraseCount = 0;

    if (!mIsFloat && mTabs.size() == 1)
    {
        // reject close request on last tab in ground area
        if (mTabs[0]->shouldClose)
            mTabs[0]->shouldClose = false;
    }
    else
    {
        for (AreaTab* tab : mTabs)
        {
            if (!tab || !tab->shouldClose)
                continue;
            
            if (tab == mActiveTab)
                mActiveTab = nullptr;

            tab->client.hide();

            if (tab->onClientClose)
                tab->onClientClose(tab->client, tab->user);

            eraseCount++;

            toErase.insert(tab);
            heap_delete<AreaTab>(tab);
        }
    }

    size_t count = std::erase_if(mTabs, [&](AreaTab* tab) { return toErase.contains(tab); });
    LD_ASSERT(eraseCount == count);

    if (!mTabs.empty() && mActiveTab == nullptr)
        mActiveTab = mTabs.front();
}

void AreaTabControl::on_update(UIWidget widget, float delta)
{
    AreaTabControl& self = *(AreaTabControl*)widget.get_user();

    // some animations for floating clients
    if (self.mIsFloat)
    {
        // closing last tab of a floating area should trigger the hide animation
        bool lastTabClosing = self.get_tab_count() == 1 && self.get_active_tab()->shouldClose;
        if (lastTabClosing && !self.mOpacityA.is_hiding())
            self.hide();

        bool isHiding = self.mOpacityA.is_hiding();
        bool animEnded = self.mOpacityA.update(delta);

        Color mask = self.mOpacityA.get_color_mask();
        self.mWindow.set_color_mask(mask);

        if (self.mActiveTab && self.mActiveTab->client)
            self.mActiveTab->client.set_color_mask(mask);

        // hide floating area after fade out animation
        if (isHiding && animEnded)
        {
            self.mWindow.hide();

            for (AreaTab* tab : self.mTabs)
            {
                if (tab->client)
                    tab->client.hide();
            }
        }
    }

    self.delete_tabs();
}

void AreaTabControl::on_float_drag(UIWidget widget, MouseButton btn, const Vec2& dragPos, bool begin)
{
    AreaTabControl& self = *(AreaTabControl*)widget.get_user();
    UIWindow window((UIWindowObj*)widget.unwrap());
    Rect windowRect = widget.get_rect();
    Vec2 mousePos = self.mCtx.get_mouse_pos();

    float distToBot, distToLeft, distToRight;
    windowRect.get_edge_distances(mousePos, &distToLeft, nullptr, &distToRight, &distToBot);

    if (begin)
    {
        self.mDragMove = false;
        self.mDragResize = false;
        self.mDragOffset = dragPos - windowRect.get_pos();
        self.mDragBeginPos = dragPos;
        self.mDragBeginSize = windowRect.get_size();

        if (distToBot < self.mFloatBorder || distToLeft < self.mFloatBorder || distToRight < self.mFloatBorder)
            self.mDragResize = true;
        else
            self.mDragMove = true;
    }

    Vec2 delta = dragPos - self.mDragBeginPos;

    if (btn == MOUSE_BUTTON_LEFT)
    {
        if (self.mDragResize)
        {
            window.set_size(self.mDragBeginSize + delta);
            Rect clientArea = self.get_float_client_area(window.get_rect());
            self.invalidate_client_area(clientArea);
        }
        else if (self.mDragMove)
        {
            window.set_pos(dragPos - self.mDragOffset);
            Rect clientArea = self.get_float_client_area(window.get_rect());
            self.invalidate_client_pos(clientArea.get_pos());
        }
    }
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

void AreaTabControl::on_float_hover(UIWidget widget, UIEvent event)
{
    Application app = Application::get();
    AreaTabControl& self = *(AreaTabControl*)widget.get_user();
    Vec2 mousePos = self.mCtx.get_mouse_pos();
    Rect windowRect = widget.get_rect();

    if (event == UI_MOUSE_ENTER)
    {
        float distToBot, distToLeft, distToRight;
        windowRect.get_edge_distances(mousePos, &distToLeft, nullptr, &distToRight, &distToBot);

        if (distToBot < self.mFloatBorder)
            app.hint_cursor_shape(CURSOR_TYPE_VRESIZE);
        else if ((distToLeft < self.mFloatBorder) || (distToRight < self.mFloatBorder))
            app.hint_cursor_shape(CURSOR_TYPE_HRESIZE);
    }
    else if (event == UI_MOUSE_LEAVE)
    {
        app.hint_cursor_shape(CURSOR_TYPE_DEFAULT);
    }
}

AreaTab::AreaTab(UIWindowManagerObj* wm, UIWindow client, UIWindow tabControl, void* user)
    : wm(wm), client(client), onClientResize(nullptr), onClientClose(nullptr), shouldClose(false), user(user)
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

    layoutI.sizeX = UISize::fixed(WINDOW_TAB_HEIGHT);
    layoutI.sizeY = UISize::fixed(WINDOW_TAB_HEIGHT);
    UIImageWidgetInfo imageWI{};
    imageWI.image = wm->get_icon_atlas();
    imageWI.rect = &wm->icons.close;
    closeW = rootW.node().add_image(layoutI, imageWI, this);
    closeW.set_on_mouse(&AreaTab::on_close);
    closeW.set_on_hover([](UIWidget widget, UIEvent event) {});
    closeW.set_on_draw([](UIWidget widget, ScreenRenderComponent renderer) {
        UITheme theme = widget.get_theme();
        if (widget.is_hovered())
            renderer.draw_rect(widget.get_rect(), theme.get_field_color());
        UIImageWidget::on_draw(widget, renderer);
    });
}

void AreaTab::on_close(UIWidget widget, const Vec2& pos, MouseButton btn, UIEvent event)
{
    if (event != UI_MOUSE_DOWN || btn != MOUSE_BUTTON_LEFT)
        return;

    AreaTab& self = *(AreaTab*)widget.get_user();

    // see AreaTabControl::on_update, tab may not be closed immediately
    // if there are animations.
    self.shouldClose = true;
}

} // namespace LD