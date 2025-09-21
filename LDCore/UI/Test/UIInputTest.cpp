#include "UITest.h"
#include <Extra/doctest/doctest.h>
#include <Ludens/System/Memory.h>

static int sLastKey;

TEST_CASE("UIWidget BlockInput")
{
    UIContext ctx = UITest::create_test_context();

    UILayoutInfo layoutI{};
    layoutI.childAxis = UI_AXIS_X;
    layoutI.sizeX = UISize::fixed(100);
    layoutI.sizeY = UISize::fixed(100);
    UIWindowInfo windowI{};
    UIWindow window = ctx.add_window(layoutI, windowI, nullptr);
    window.set_on_key([](UIWidget widget, KeyCode key, UIEvent event) {
        sLastKey = key;
    });

    window.layout();

    sLastKey = 0;
    ctx.input_mouse_position(Vec2(50, 50));
    ctx.input_key_down(KEY_CODE_A);
    CHECK(sLastKey == KEY_CODE_A);

    window.block_input();
    ctx.input_key_down(KEY_CODE_B);
    CHECK(sLastKey == KEY_CODE_A);

    window.unblock_input();
    ctx.input_key_down(KEY_CODE_C);
    CHECK(sLastKey == KEY_CODE_C);

    UIContext::destroy(ctx);
}
