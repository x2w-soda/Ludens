#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <Extra/doctest/doctest.h>
#include <Ludens/UI/UI.h>

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
        return UIContext::create();
    }
};

struct Text
{
    const char* cstr;
    const float glyphW = 10.0f;
    const float glyphH = 10.0f;

    static void wrap_limit_fn(void* user, float& outMin, float& outMax)
    {
        Text& text = *(Text*)user;

        size_t len = strlen(text.cstr);

        // for testing we wrap on glyph boundaries instead of word boundaries
        outMin = text.glyphW;
        outMax = len * text.glyphW;
    }

    static float wrap_size_fn(void* user, float limitW)
    {
        Text& text = *(Text*)user;

        // this is where we use font metrics
        // to layout text, for testing we assume monospace
        // glyph sizes without advanceX, kerning, and other
        // common font metrics.
        float heightY = text.glyphH;
        float width = 0;

        size_t len = strlen(text.cstr);

        // usually we line-break at whitespace but this is just for testing.
        for (size_t i = 0; i < len; i++)
        {
            if (width + text.glyphW > limitW)
            {
                heightY += text.glyphH;
                width = 0;
            }

            width += text.glyphW;
        }

        return heightY;
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
    UIElement child = window.add_child(layoutI, nullptr);

    ctx.layout();

    Rect rect = window.get_rect();
    CHECK(rect == Rect(0, 0, 164, 164));

    rect = child.get_rect();
    CHECK(rect == Rect(32, 32, 100, 100));

    UIContext::destroy(ctx);
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
    UIElement hbox = window.add_child(layoutI, nullptr);

    layoutI.sizeX = UISize::grow();
    layoutI.sizeY = UISize::fixed(20.0f);
    UIElement c1 = hbox.add_child(layoutI, nullptr);
    UIElement c2 = hbox.add_child(layoutI, nullptr);
    UIElement c3 = hbox.add_child(layoutI, nullptr);

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
    UIElement hbox = window.add_child(layoutI, nullptr);

    layoutI.sizeX = UISize::fixed(20.0f);
    layoutI.sizeY = UISize::grow();
    UIElement c1 = hbox.add_child(layoutI, nullptr);
    UIElement c2 = hbox.add_child(layoutI, nullptr);
    UIElement c3 = hbox.add_child(layoutI, nullptr);

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
}

TEST_CASE("UILayout size wrapping")
{
    UIContext ctx = UITest::create_test_context();

    UILayoutInfo layoutI = make_fixed_size_layout(100, 100);
    layoutI.childPadding = {20, 20, 20, 20};
    layoutI.childAxis = UI_AXIS_Y;

    UIWindowInfo windowI{};
    windowI.name = "test_window";
    UIWindow window = ctx.add_window(layoutI, windowI, nullptr);

    Text text{"some text"}; // 90x10
    layoutI.sizeX = UISize::wrap_primary(&Text::wrap_size_fn, &Text::wrap_limit_fn);
    layoutI.sizeY = UISize::wrap_secondary();
    UIElement wrap = window.add_child(layoutI, &text);

    ctx.layout();

    // 90x10 should wrap to 60x20 using 10x10 glyph as wrap boundary
    Rect rect = window.get_rect();
    CHECK(rect == Rect(0, 0, 100, 100));

    rect = wrap.get_rect();
    CHECK(rect == Rect(20, 20, 60, 20));

    text.cstr = "hello, world!"; // 130x10
    ctx.layout();

    // 130x10 should wrap to 60x30 using 10x10 glyph as wrap boundary
    rect = wrap.get_rect();
    CHECK(rect == Rect(20, 20, 60, 30));

    UIContext::destroy(ctx);
}