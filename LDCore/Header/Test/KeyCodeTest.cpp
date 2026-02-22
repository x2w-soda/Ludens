#include <Extra/doctest/doctest.h>
#include <Ludens/Header/KeyValue.h>

using namespace LD;

TEST_CASE("KeyValue")
{
    KeyValue v1;
    CHECK(v1.u32 == 0);

    v1 = KeyValue(KEY_CODE_SPACE);
    CHECK(v1.u32 == KEY_CODE_SPACE); // no mods
    CHECK(v1.code() == KEY_CODE_SPACE);
    CHECK(v1.mods() == 0);

    KeyValue v2(KEY_CODE_SPACE, KEY_MOD_CONTROL_BIT);
    CHECK(v2 == v2);
    CHECK(v2.code() == KEY_CODE_SPACE);
    CHECK(v2.mods() == KEY_MOD_CONTROL_BIT);
    CHECK(v1 != v2);

    KeyValue v3(KEY_CODE_SPACE, KEY_MOD_CONTROL_BIT | KEY_MOD_SHIFT_BIT);
    CHECK(v3.code() == KEY_CODE_SPACE);
    CHECK(v3.mods() == (KEY_MOD_CONTROL_BIT | KEY_MOD_SHIFT_BIT));
    CHECK(v2 != v3);

    KeyValue v4(KEY_CODE_A, KEY_MOD_CONTROL_BIT);
    CHECK(v2.code() != v4.code());
    CHECK(v2.mods() == v4.mods());
    CHECK(v2 != v4);
}

// TODO: MouseValue