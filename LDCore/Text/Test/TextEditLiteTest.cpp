#include <Extra/doctest/doctest.h>
#include <Ludens/Memory/Memory.h>
#include <Ludens/Text/TextEditLite.h>

using namespace LD;

TEST_CASE("TextEditLite printable ascii")
{
    TextEditLite editor = TextEditLite::create();

    for (char c = KEY_CODE_SPACE; c < KEY_CODE_GRAVE_ACCENT; c++)
    {
        editor.clear();
        editor.key(KeyValue((KeyCode)c, 0));

        std::string expected(1, c);
        if ('A' <= c && c <= 'Z')
            expected.back() += 32;

        CHECK(editor.get_string() == expected);
    }

    TextEditLite::destroy(editor);
    CHECK_FALSE(get_memory_leaks(nullptr));
}

TEST_CASE("TextEditLite shift mod")
{
    TextEditLite editor = TextEditLite::create();

    CHECK(editor.key(KeyValue(KEY_CODE_1, KEY_MOD_SHIFT_BIT)) == TEXT_EDIT_LITE_RESULT_CHANGED);
    CHECK(editor.get_string() == "!");
    CHECK(editor.key(KeyValue(KEY_CODE_2, KEY_MOD_SHIFT_BIT)) == TEXT_EDIT_LITE_RESULT_CHANGED);
    CHECK(editor.get_string() == "!@");
    CHECK(editor.key(KeyValue(KEY_CODE_3, KEY_MOD_SHIFT_BIT)) == TEXT_EDIT_LITE_RESULT_CHANGED);
    CHECK(editor.get_string() == "!@#");
    CHECK(editor.key(KeyValue(KEY_CODE_4, KEY_MOD_SHIFT_BIT)) == TEXT_EDIT_LITE_RESULT_CHANGED);
    CHECK(editor.get_string() == "!@#$");
    CHECK(editor.key(KeyValue(KEY_CODE_5, KEY_MOD_SHIFT_BIT)) == TEXT_EDIT_LITE_RESULT_CHANGED);
    CHECK(editor.get_string() == "!@#$%");
    CHECK(editor.key(KeyValue(KEY_CODE_6, KEY_MOD_SHIFT_BIT)) == TEXT_EDIT_LITE_RESULT_CHANGED);
    CHECK(editor.get_string() == "!@#$%^");
    CHECK(editor.key(KeyValue(KEY_CODE_7, KEY_MOD_SHIFT_BIT)) == TEXT_EDIT_LITE_RESULT_CHANGED);
    CHECK(editor.get_string() == "!@#$%^&");
    CHECK(editor.key(KeyValue(KEY_CODE_8, KEY_MOD_SHIFT_BIT)) == TEXT_EDIT_LITE_RESULT_CHANGED);
    CHECK(editor.get_string() == "!@#$%^&*");
    CHECK(editor.key(KeyValue(KEY_CODE_9, KEY_MOD_SHIFT_BIT)) == TEXT_EDIT_LITE_RESULT_CHANGED);
    CHECK(editor.get_string() == "!@#$%^&*(");
    CHECK(editor.key(KeyValue(KEY_CODE_0, KEY_MOD_SHIFT_BIT)) == TEXT_EDIT_LITE_RESULT_CHANGED);
    CHECK(editor.get_string() == "!@#$%^&*()");

    CHECK(editor.get_cursor() == 10);
    editor.clear();

    CHECK(editor.key(KeyValue(KEY_CODE_KEYPAD_1, KEY_MOD_SHIFT_BIT)) == TEXT_EDIT_LITE_RESULT_CHANGED);
    CHECK(editor.get_string() == "1");
    CHECK(editor.key(KeyValue(KEY_CODE_KEYPAD_2, KEY_MOD_SHIFT_BIT)) == TEXT_EDIT_LITE_RESULT_CHANGED);
    CHECK(editor.get_string() == "12");
    CHECK(editor.key(KeyValue(KEY_CODE_KEYPAD_3, KEY_MOD_SHIFT_BIT)) == TEXT_EDIT_LITE_RESULT_CHANGED);
    CHECK(editor.get_string() == "123");
    CHECK(editor.key(KeyValue(KEY_CODE_KEYPAD_4, KEY_MOD_SHIFT_BIT)) == TEXT_EDIT_LITE_RESULT_CHANGED);
    CHECK(editor.get_string() == "1234");
    CHECK(editor.key(KeyValue(KEY_CODE_KEYPAD_5, KEY_MOD_SHIFT_BIT)) == TEXT_EDIT_LITE_RESULT_CHANGED);
    CHECK(editor.get_string() == "12345");
    CHECK(editor.key(KeyValue(KEY_CODE_KEYPAD_6, KEY_MOD_SHIFT_BIT)) == TEXT_EDIT_LITE_RESULT_CHANGED);
    CHECK(editor.get_string() == "123456");
    CHECK(editor.key(KeyValue(KEY_CODE_KEYPAD_7, KEY_MOD_SHIFT_BIT)) == TEXT_EDIT_LITE_RESULT_CHANGED);
    CHECK(editor.get_string() == "1234567");
    CHECK(editor.key(KeyValue(KEY_CODE_KEYPAD_8, KEY_MOD_SHIFT_BIT)) == TEXT_EDIT_LITE_RESULT_CHANGED);
    CHECK(editor.get_string() == "12345678");
    CHECK(editor.key(KeyValue(KEY_CODE_KEYPAD_9, KEY_MOD_SHIFT_BIT)) == TEXT_EDIT_LITE_RESULT_CHANGED);
    CHECK(editor.get_string() == "123456789");
    CHECK(editor.key(KeyValue(KEY_CODE_KEYPAD_0, KEY_MOD_SHIFT_BIT)) == TEXT_EDIT_LITE_RESULT_CHANGED);
    CHECK(editor.get_string() == "1234567890");

    CHECK(editor.get_cursor() == 10);
    editor.clear();

    CHECK(editor.key(KeyValue(KEY_CODE_LEFT_BRACKET, KEY_MOD_SHIFT_BIT)) == TEXT_EDIT_LITE_RESULT_CHANGED);
    CHECK(editor.get_string() == "{");
    CHECK(editor.key(KeyValue(KEY_CODE_RIGHT_BRACKET, KEY_MOD_SHIFT_BIT)) == TEXT_EDIT_LITE_RESULT_CHANGED);
    CHECK(editor.get_string() == "{}");
    CHECK(editor.key(KeyValue(KEY_CODE_BACKSLASH, KEY_MOD_SHIFT_BIT)) == TEXT_EDIT_LITE_RESULT_CHANGED);
    CHECK(editor.get_string() == "{}|");
    CHECK(editor.key(KeyValue(KEY_CODE_SLASH, KEY_MOD_SHIFT_BIT)) == TEXT_EDIT_LITE_RESULT_CHANGED);
    CHECK(editor.get_string() == "{}|?");
    CHECK(editor.key(KeyValue(KEY_CODE_APOSTROPHE, KEY_MOD_SHIFT_BIT)) == TEXT_EDIT_LITE_RESULT_CHANGED);
    CHECK(editor.get_string() == "{}|?\"");
    CHECK(editor.key(KeyValue(KEY_CODE_GRAVE_ACCENT, KEY_MOD_SHIFT_BIT)) == TEXT_EDIT_LITE_RESULT_CHANGED);
    CHECK(editor.get_string() == "{}|?\"~");

    TextEditLite::destroy(editor);
    CHECK_FALSE(get_memory_leaks(nullptr));
}