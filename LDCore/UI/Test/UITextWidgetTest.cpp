#include "UITest.h"
#include <Extra/doctest/doctest.h>
#include <Ludens/System/Memory.h>

TEST_CASE("UITextWidget in fit container" * doctest::skip(!UITest::found_lfs_directory()))
{
    UIWorkspace space;
    UIContext ctx = UITest::create_test_context(Vec2(100.0f, 100.0f), space);

    UILayoutInfo layoutI{};
    layoutI.childAxis = UI_AXIS_X;
    layoutI.sizeX = UISize::fit();
    layoutI.sizeY = UISize::fit();
    UIWindowInfo windowI{};
    UIWindow window = space.create_window(space.get_root_id(), layoutI, windowI, nullptr);

    UITextWidgetInfo textWI{};
    textWI.cstr = "foo";
    textWI.fontSize = 16;
    textWI.hoverHL = false;
    UITextWidget textW1 = window.node().add_text({}, textWI, nullptr);
    textWI.cstr = "bar";
    UITextWidget textW2 = window.node().add_text({}, textWI, nullptr);

    ctx.update(0.0f);

    Rect rect = textW1.get_rect();
    CHECK(rect.x == 0);
    CHECK(rect.y == 0);
    CHECK(rect.w > 0);
    CHECK(rect.h > 0);

    rect = textW2.get_rect();
    CHECK(rect.x > 0);
    CHECK(rect.y == 0);
    CHECK(rect.w > 0);
    CHECK(rect.h > 0);

    window.set_layout_child_axis(UI_AXIS_Y);

    ctx.update(0.0f);

    rect = textW1.get_rect();
    CHECK(rect.x == 0);
    CHECK(rect.y == 0);
    CHECK(rect.w > 0);
    CHECK(rect.h > 0);

    rect = textW2.get_rect();
    CHECK(rect.x == 0);
    CHECK(rect.y > 0);
    CHECK(rect.w > 0);
    CHECK(rect.h > 0);

    UIContext::destroy(ctx);
}
