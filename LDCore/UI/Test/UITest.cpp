#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <Extra/doctest/doctest.h>
#include <Ludens/System/Memory.h>
#include <Ludens/UI/UIContext.h>
#include <Ludens/UI/UILayout.h>

using namespace LD;

inline UILayoutInfo make_fit_layout()
{
    UILayoutInfo layoutI{};
    layoutI.childAxis = UI_AXIS_X;
    layoutI.childGap = 0.0f;
    layoutI.sizeX = UISize::fit();
    layoutI.sizeY = UISize::fit();
    return layoutI;
}

inline UILayoutInfo make_fixed_size_layout(float sizeX, float sizeY)
{
    UILayoutInfo layoutI{};
    layoutI.childAxis = UI_AXIS_X;
    layoutI.childGap = 0.0f;
    layoutI.sizeX = UISize::fixed(sizeX);
    layoutI.sizeY = UISize::fixed(sizeY);
    return layoutI;
}

struct UITest
{
    static UIContext create_test_context()
    {
        static UIThemeInfo sTheme = UITheme::get_default_info();

        UIContextInfo ctxI;
        ctxI.fontAtlas = {};
        ctxI.fontAtlasImage = {};
        ctxI.theme = UITheme(&sTheme);
        return UIContext::create(ctxI);
    }
};

TEST_CASE("UILayout window padding")
{
    UIContext ctx = UITest::create_test_context();

    UILayoutInfo layoutI = make_fit_layout();
    layoutI.childPadding = {32, 32, 32, 32};
    layoutI.childAxis = UI_AXIS_Y;

    UIWindowInfo windowI{};
    windowI.name = "test_window";
    UIWindow window = ctx.add_window(layoutI, windowI, nullptr);

    layoutI = make_fixed_size_layout(100, 100);
    UIPanelWidgetInfo panelI{};
    UIPanelWidget child = window.node().add_panel(layoutI, panelI, nullptr);

    ctx.layout();

    Rect rect = window.get_rect();
    CHECK(rect == Rect(0, 0, 164, 164));

    rect = child.get_rect();
    CHECK(rect == Rect(32, 32, 100, 100));

    UIContext::destroy(ctx);
    int leaks = get_memory_leaks(nullptr);
    CHECK(leaks == 0);
}

TEST_CASE("UILayout hbox child grows x")
{
    UIContext ctx = UITest::create_test_context();

    UILayoutInfo layoutI = make_fit_layout();
    layoutI.childPadding = {10, 10, 10, 10};
    UIWindowInfo windowI{};
    windowI.name = "test_window";
    UIWindow window = ctx.add_window(layoutI, windowI, nullptr);

    // horizontal container
    layoutI = make_fixed_size_layout(150, 150);
    layoutI.childAxis = UI_AXIS_X;
    layoutI.childPadding = {};
    UIPanelWidgetInfo panelI{};
    UIPanelWidget hbox = window.node().add_panel(layoutI, panelI, nullptr);

    layoutI.sizeX = UISize::grow();
    layoutI.sizeY = UISize::fixed(20.0f);
    UIPanelWidget c1 = hbox.node().add_panel(layoutI, panelI, nullptr);
    UIPanelWidget c2 = hbox.node().add_panel(layoutI, panelI, nullptr);
    UIPanelWidget c3 = hbox.node().add_panel(layoutI, panelI, nullptr);

    ctx.layout();

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
    int leaks = get_memory_leaks(nullptr);
    CHECK(leaks == 0);
}

TEST_CASE("UILayout hbox child grows y")
{
    UIContext ctx = UITest::create_test_context();

    UILayoutInfo layoutI = make_fit_layout();
    layoutI.childPadding = {10, 10, 10, 10};
    UIWindowInfo windowI{};
    windowI.name = "test_window";
    UIWindow window = ctx.add_window(layoutI, windowI, nullptr);

    // horizontal container
    layoutI = make_fixed_size_layout(150, 150);
    layoutI.childAxis = UI_AXIS_X;
    layoutI.childPadding = {};
    UIPanelWidgetInfo panelI{};
    UIPanelWidget hbox = window.node().add_panel(layoutI, panelI, nullptr);

    layoutI.sizeX = UISize::fixed(20.0f);
    layoutI.sizeY = UISize::grow();
    UIPanelWidget c1 = hbox.node().add_panel(layoutI, panelI, nullptr);
    UIPanelWidget c2 = hbox.node().add_panel(layoutI, panelI, nullptr);
    UIPanelWidget c3 = hbox.node().add_panel(layoutI, panelI, nullptr);

    ctx.layout();

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
    int leaks = get_memory_leaks(nullptr);
    CHECK(leaks == 0);
}
