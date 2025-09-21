#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "UITest.h"
#include <Extra/doctest/doctest.h>
#include <Ludens/Header/Assert.h>
#include <Ludens/System/Memory.h>
#include <Ludens/UI/UIContext.h>
#include <Ludens/UI/UILayout.h>

UITest* UITest::sInstance;

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
    const MemoryProfile& profile = get_memory_profile(MEMORY_USAGE_UI);
    CHECK(profile.current == 0);
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
    const MemoryProfile& profile = get_memory_profile(MEMORY_USAGE_UI);
    CHECK(profile.current == 0);
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
    const MemoryProfile& profile = get_memory_profile(MEMORY_USAGE_UI);
    CHECK(profile.current == 0);
}

TEST_CASE("UILayout nested grow")
{
    UIContext ctx = UITest::create_test_context();

    UILayoutInfo layoutI = make_fit_layout();
    layoutI.childPadding = {};
    layoutI.childAxis = UI_AXIS_X;
    layoutI.sizeX = UISize::fixed(500);
    layoutI.sizeY = UISize::fixed(500);
    UIWindowInfo windowI{};
    UIWindow window = ctx.add_window(layoutI, windowI, nullptr);

    layoutI.sizeX = UISize::grow();
    layoutI.sizeY = UISize::grow();
    UIPanelWidgetInfo panelWI{};
    UIPanelWidget p1 = window.node().add_panel(layoutI, panelWI, nullptr);
    UIPanelWidget p2 = p1.node().add_panel(layoutI, panelWI, nullptr); // also growing

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

UITest::UITest()
{
    mTheme = UITheme::get_default_info();
    mLFSDirectoryPath.clear();
    mFont = {};
    mFontAtlas = {};

    std::cout << std::filesystem::current_path() << std::endl;

    FS::Path lfsPath;
    if (get_lfs_directory(lfsPath))
    {
        std::cout << "found lfs directory at: " << lfsPath << std::endl;

        FS::Path fontPath = lfsPath / FS::Path("Fonts/Inter_24pt-Regular.ttf");
        LD_ASSERT(FS::exists(fontPath));

        std::string pathString = fontPath.string();
        mFont = Font::create_from_path(pathString.c_str());
        mFontAtlas = FontAtlas::create_bitmap(mFont, 24);
    }
}

UITest* UITest::get()
{
    if (!sInstance)
        sInstance = new UITest();

    return sInstance;
}

bool UITest::get_lfs_directory(FS::Path& lfsDirectory)
{
    const char* candidates[] = {
        "../../../../Ludens/Extra/LudensLFS",
        "../../../../../Ludens/Extra/LudensLFS",
    };

    for (const char* candidate : candidates)
    {
        if (FS::exists(candidate))
        {
            lfsDirectory = candidate;
            return true;
        }
    }

    return false;
}
