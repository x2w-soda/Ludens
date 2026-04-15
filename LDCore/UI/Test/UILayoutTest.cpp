#include <Extra/doctest/doctest.h>
#include <Ludens/UI/Widget/UIPanelWidget.h>

#include "UITest.h"

TEST_CASE("UILayout alignment")
{
    UIWorkspace space;
    UIContext ctx = UITest::create_test_context(Vec2(100.0f, 100.0f), space);

    // main axis X
    {
        UILayoutInfo layoutI{};
        layoutI.sizeX = UISize::fixed(100);
        layoutI.sizeY = UISize::fixed(100);
        layoutI.childPadding = UIPadding::left_right(10.0f, 30.0f); // alignment considers width of 60 along X
        layoutI.childAxis = UI_AXIS_X;
        layoutI.childAlignX = UI_ALIGN_BEGIN;
        layoutI.childAlignY = UI_ALIGN_CENTER;
        layoutI.childGap = 10.0f;
        UIWindowInfo windowI{};
        UIWindow window = space.create_window(space.get_root_id(), layoutI, windowI, nullptr);

        layoutI.sizeX = UISize::fixed(20);
        layoutI.sizeY = UISize::fixed(40);
        UIPanelWidget p1 = (UIPanelWidget)window.add_child(UI_WIDGET_PANEL, layoutI, nullptr, nullptr);
        UIPanelWidget p2 = (UIPanelWidget)window.add_child(UI_WIDGET_PANEL, layoutI, nullptr, nullptr);
        window.layout();

        Vec2 pos = p1.get_pos();
        CHECK(pos == Vec2(10, 30));
        pos = p2.get_pos();
        CHECK(pos == Vec2(40, 30));

        window.set_layout_child_align_x(UI_ALIGN_END);
        window.layout();

        pos = p1.get_pos();
        CHECK(pos == Vec2(20, 30));
        pos = p2.get_pos();
        CHECK(pos == Vec2(50, 30));

        window.set_layout_child_align_x(UI_ALIGN_CENTER);
        window.layout();

        pos = p1.get_pos();
        CHECK(pos == Vec2(15, 30));
        pos = p2.get_pos();
        CHECK(pos == Vec2(45, 30));

        space.destroy_window(window);
    }

    // main axis Y
    {
        UILayoutInfo layoutI{};
        layoutI.sizeX = UISize::fixed(100);
        layoutI.sizeY = UISize::fixed(100);
        layoutI.childPadding = UIPadding::top_bottom(10.0f, 30.0f); // alignment considers width of 60 along Y
        layoutI.childAxis = UI_AXIS_Y;
        layoutI.childAlignX = UI_ALIGN_CENTER;
        layoutI.childAlignY = UI_ALIGN_BEGIN;
        layoutI.childGap = 10.0f;
        UIWindowInfo windowI{};
        UIWindow window = space.create_window(space.get_root_id(), layoutI, windowI, nullptr);

        layoutI.sizeX = UISize::fixed(40);
        layoutI.sizeY = UISize::fixed(20);
        UIPanelWidget p1 = (UIPanelWidget)window.add_child(UI_WIDGET_PANEL, layoutI, nullptr, nullptr);
        UIPanelWidget p2 = (UIPanelWidget)window.add_child(UI_WIDGET_PANEL, layoutI, nullptr, nullptr);

        window.layout();

        Vec2 pos = p1.get_pos();
        CHECK(pos == Vec2(30, 10));
        pos = p2.get_pos();
        CHECK(pos == Vec2(30, 40));

        window.set_layout_child_align_y(UI_ALIGN_END);
        window.layout();

        pos = p1.get_pos();
        CHECK(pos == Vec2(30, 20));
        pos = p2.get_pos();
        CHECK(pos == Vec2(30, 50));

        window.set_layout_child_align_y(UI_ALIGN_CENTER);
        window.layout();

        pos = p1.get_pos();
        CHECK(pos == Vec2(30, 15));
        pos = p2.get_pos();
        CHECK(pos == Vec2(30, 45));

        space.destroy_window(window);
    }

    UIContext::destroy(ctx);
    const MemoryProfile& profile = get_memory_profile(MEMORY_USAGE_UI);
    CHECK(profile.current == 0);
}

TEST_CASE("UILayout window childPad")
{
    UIWorkspace space;
    UIContext ctx = UITest::create_test_context(Vec2(164.0f, 164.0f), space);

    UILayoutInfo layoutI = make_fit_layout();
    layoutI.childPadding = {32, 32, 32, 32};
    layoutI.childAxis = UI_AXIS_Y;

    UIWindowInfo windowI{};
    windowI.name = "test_window";
    UIWindow window = space.create_window(space.get_root_id(), layoutI, windowI, nullptr);

    layoutI = make_fixed_size_layout(100, 100);
    UIPanelWidget child = (UIPanelWidget)window.add_child(UI_WIDGET_PANEL, layoutI, nullptr, nullptr);

    ctx.update(0.0f);

    Rect rect = window.get_rect();
    CHECK(rect == Rect(0, 0, 164, 164));

    rect = child.get_rect();
    CHECK(rect == Rect(32, 32, 100, 100));

    UIContext::destroy(ctx);
    const MemoryProfile& profile = get_memory_profile(MEMORY_USAGE_UI);
    CHECK(profile.current == 0);
}

TEST_CASE("UILayout hbox child grows x")
{
    UIWorkspace space;
    UIContext ctx = UITest::create_test_context(Vec2(170.0f, 170.0f), space);

    UILayoutInfo layoutI = make_fit_layout();
    layoutI.childPadding = {10, 10, 10, 10};
    UIWindowInfo windowI{};
    windowI.name = "test_window";
    UIWindow window = space.create_window(space.get_root_id(), layoutI, windowI, nullptr);

    // horizontal container
    layoutI = make_fixed_size_layout(150, 150);
    layoutI.childAxis = UI_AXIS_X;
    layoutI.childPadding = {};
    UIPanelWidget hbox = (UIPanelWidget)window.add_child(UI_WIDGET_PANEL, layoutI, nullptr, nullptr);

    layoutI.sizeX = UISize::grow();
    layoutI.sizeY = UISize::fixed(20.0f);
    UIPanelWidget c1 = (UIPanelWidget)hbox.add_child(UI_WIDGET_PANEL, layoutI, nullptr, nullptr);
    UIPanelWidget c2 = (UIPanelWidget)hbox.add_child(UI_WIDGET_PANEL, layoutI, nullptr, nullptr);
    UIPanelWidget c3 = (UIPanelWidget)hbox.add_child(UI_WIDGET_PANEL, layoutI, nullptr, nullptr);

    ctx.update(0.0f);

    Rect rect = window.get_rect();
    CHECK(rect == Rect(0, 0, 170, 170));
    rect = hbox.get_rect();
    CHECK(rect == Rect(10, 10, 150, 150));
    rect = c1.get_rect();
    CHECK(rect == Rect(10, 10, 50, 20));
    rect = c2.get_rect();
    CHECK(rect == Rect(60, 10, 50, 20));
    rect = c3.get_rect();
    CHECK(rect == Rect(110, 10, 50, 20));

    UIContext::destroy(ctx);
    const MemoryProfile& profile = get_memory_profile(MEMORY_USAGE_UI);
    CHECK(profile.current == 0);
}

TEST_CASE("UILayout hbox child grows y")
{
    UIWorkspace space;
    UIContext ctx = UITest::create_test_context(Vec2(170.0f, 170.0f), space);

    UILayoutInfo layoutI = make_fit_layout();
    layoutI.childPadding = {10, 10, 10, 10};
    UIWindowInfo windowI{};
    windowI.name = "test_window";
    UIWindow window = space.create_window(space.get_root_id(), layoutI, windowI, nullptr);

    // horizontal container
    layoutI = make_fixed_size_layout(150, 150);
    layoutI.childAxis = UI_AXIS_X;
    layoutI.childPadding = {};
    UIPanelWidget hbox = (UIPanelWidget)window.add_child(UI_WIDGET_PANEL, layoutI, nullptr, nullptr);

    layoutI.sizeX = UISize::fixed(20.0f);
    layoutI.sizeY = UISize::grow();
    UIPanelWidget c1 = (UIPanelWidget)hbox.add_child(UI_WIDGET_PANEL, layoutI, nullptr, nullptr);
    UIPanelWidget c2 = (UIPanelWidget)hbox.add_child(UI_WIDGET_PANEL, layoutI, nullptr, nullptr);
    UIPanelWidget c3 = (UIPanelWidget)hbox.add_child(UI_WIDGET_PANEL, layoutI, nullptr, nullptr);

    ctx.update(0.0f);

    Rect rect = window.get_rect();
    CHECK(rect == Rect(0, 0, 170, 170));
    rect = hbox.get_rect();
    CHECK(rect == Rect(10, 10, 150, 150));
    rect = c1.get_rect();
    CHECK(rect == Rect(10, 10, 20, 150));
    rect = c2.get_rect();
    CHECK(rect == Rect(30, 10, 20, 150));
    rect = c3.get_rect();
    CHECK(rect == Rect(50, 10, 20, 150));

    UIContext::destroy(ctx);
    const MemoryProfile& profile = get_memory_profile(MEMORY_USAGE_UI);
    CHECK(profile.current == 0);
}

TEST_CASE("UILayout nested grow")
{
    UIWorkspace space;
    UIContext ctx = UITest::create_test_context(Vec2(500.0f, 500.0f), space);

    UILayoutInfo layoutI = make_fit_layout();
    layoutI.childPadding = {};
    layoutI.childAxis = UI_AXIS_X;
    layoutI.sizeX = UISize::fixed(500);
    layoutI.sizeY = UISize::fixed(500);
    UIWindowInfo windowI{};
    UIWindow window = space.create_window(space.get_root_id(), layoutI, windowI, nullptr);

    layoutI.sizeX = UISize::grow();
    layoutI.sizeY = UISize::grow();
    UIPanelWidget p1 = (UIPanelWidget)window.add_child(UI_WIDGET_PANEL, layoutI, nullptr, nullptr);
    UIPanelWidget p2 = (UIPanelWidget)p1.add_child(UI_WIDGET_PANEL, layoutI, nullptr, nullptr); // also growing

    window.layout();

    Rect r1 = p1.get_rect();
    CHECK(r1.get_size() == Vec2(500, 500));
    Rect r2 = p1.get_rect();
    CHECK(r2.get_size() == Vec2(500, 500));

    // increase fixed size
    window.set_size(Vec2(600, 700));
    window.layout();

    r1 = p1.get_rect();
    CHECK(r1.get_size() == Vec2(600, 700));
    r2 = p2.get_rect();
    CHECK(r2.get_size() == Vec2(600, 700));

    // decrease fixed size
    window.set_size(Vec2(300, 400));
    window.layout();

    r1 = p1.get_rect();
    CHECK(r1.get_size() == Vec2(300, 400));
    r2 = p2.get_rect();
    CHECK(r2.get_size() == Vec2(300, 400));

    UIContext::destroy(ctx);
    const MemoryProfile& profile = get_memory_profile(MEMORY_USAGE_UI);
    CHECK(profile.current == 0);
}