#include <Extra/doctest/doctest.h>
#include <Ludens/Memory/Memory.h>

#include "UITest.h"

static KeyCode sLastKey;

TEST_CASE("UIWidget input key")
{
    UIWorkspace space;
    UIContext ctx = UITest::create_test_context(Vec2(100.0f, 100.0f), space);

    UILayoutInfo layoutI{};
    layoutI.childAxis = UI_AXIS_X;
    layoutI.sizeX = UISize::fixed(100);
    layoutI.sizeY = UISize::fixed(100);
    UIWindowInfo windowI{};
    UIWindow window = space.create_window(space.get_root_id(), layoutI, windowI, nullptr);
    window.set_on_event([](UIWidget widget, const UIEvent& event) -> bool {
        if (event.type != UI_EVENT_KEY_DOWN)
            return false;

        sLastKey = event.key.code;
        return true;
    });

    window.layout();

    sLastKey = KeyCode(0);
    ctx.input_mouse_position(Vec2(50.0f, 50.0f));
    ctx.input_key_down(KEY_CODE_A, 0);
    CHECK(sLastKey == KEY_CODE_A);

    ctx.input_key_down(KEY_CODE_B, 0);
    CHECK(sLastKey == KEY_CODE_B);

    ctx.input_key_down(KEY_CODE_C, 0);
    CHECK(sLastKey == KEY_CODE_C);

    UIContext::destroy(ctx);
}
