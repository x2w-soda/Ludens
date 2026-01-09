#include "UITest.h"
#include <Extra/doctest/doctest.h>
#include <Ludens/Memory/Memory.h>

static int sLastKey;

TEST_CASE("UIWidget BlockInput")
{
    UIWorkspace space;
    UIContext ctx = UITest::create_test_context(Vec2(100.0f, 100.0f), space);

    UILayoutInfo layoutI{};
    layoutI.childAxis = UI_AXIS_X;
    layoutI.sizeX = UISize::fixed(100);
    layoutI.sizeY = UISize::fixed(100);
    UIWindowInfo windowI{};
    UIWindow window = space.create_window(space.get_root_id(), layoutI, windowI, nullptr);
    window.set_on_key([](UIWidget widget, KeyCode key, UIEvent event) {
        sLastKey = key;
    });

    window.layout();

    sLastKey = 0;
    MouseMotionEvent mm(50, 50);
    ctx.on_event(&mm);
    KeyDownEvent kdA(KEY_CODE_A, false);
    ctx.on_event(&kdA);
    CHECK(sLastKey == KEY_CODE_A);

    KeyDownEvent kdB(KEY_CODE_B, false);
    window.block_input();
    ctx.on_event(&kdB);
    CHECK(sLastKey == KEY_CODE_A);

    KeyDownEvent kdC(KEY_CODE_C, false);
    window.unblock_input();
    ctx.on_event(&kdC);
    CHECK(sLastKey == KEY_CODE_C);

    UIContext::destroy(ctx);
}
