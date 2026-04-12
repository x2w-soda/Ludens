#include <Ludens/Header/Assert.h>
#include <Ludens/Memory/Memory.h>
#include <Ludens/Text/Text.h>
#include <Ludens/Text/TextBuffer.h>
#include <Ludens/Text/TextEditLite.h>

#include <algorithm>

namespace LD {

struct TextEditLiteObj
{
    size_t cursor = 0;
    Range selection = {};
    TextBuffer buffer = {};

    void cursor_inc();
    void cursor_dec();
    bool remove_char();
    bool remove_range(Range range);
    bool remove_selection();
    bool remove();
    bool deletion();
    void add(char c);
    void select_all();
    bool exit_selection_left();
    bool exit_selection_right();
};

void TextEditLiteObj::cursor_inc()
{
    if (cursor < buffer.size())
        cursor++;
}

void TextEditLiteObj::cursor_dec()
{
    if (cursor > 0)
        cursor--;
}

bool TextEditLiteObj::remove_char()
{
    if (buffer.size() == 0 || cursor == 0)
        return false;

    buffer.erase(cursor - 1);
    cursor_dec();
    return true;
}

bool TextEditLiteObj::remove_range(Range range)
{
    size_t size = buffer.size();
    if (size < range.offset)
        return false;

    size -= range.offset;
    size = std::min(size, range.size);

    cursor = range.offset;

    for (size_t i = 0; i < size; i++)
        buffer.erase(cursor);

    return size > 0;
}

bool TextEditLiteObj::remove_selection()
{
    if (selection)
    {
        bool hasRemoved = remove_range(selection);
        selection.size = 0;
        return hasRemoved;
    }

    return false;
}

/// @brief Expected 'backspace' behavior
bool TextEditLiteObj::remove()
{
    if (remove_selection())
        return true;

    return remove_char();
}

/// @brief Expected 'delete' behavior
bool TextEditLiteObj::deletion()
{
    if (remove_selection())
        return true;

    buffer.erase(cursor);
}

void TextEditLiteObj::add(char c)
{
    if (selection)
    {
        size_t cursorPos = selection.offset;
        (void)remove_selection();
        cursor = cursorPos;
    }

    buffer.insert(cursor, c);

    cursor_inc();
}

void TextEditLiteObj::select_all()
{
    selection = Range(0, buffer.size());
    cursor = 0;
}

bool TextEditLiteObj::exit_selection_left()
{
    if (!selection)
        return false;

    cursor = selection.offset;
    selection.size = 0;
    return true;
}

bool TextEditLiteObj::exit_selection_right()
{
    if (!selection)
        return false;

    cursor = selection.offset + selection.size;
    selection.size = 0;
    return true;
}

TextEditLite TextEditLite::create()
{
    auto* obj = heap_new<TextEditLiteObj>(MEMORY_USAGE_TEXT_EDIT);
    obj->buffer = TextBuffer::create();
    obj->cursor = 0;

    return TextEditLite(obj);
}

void TextEditLite::destroy(TextEditLite editor)
{
    auto* obj = editor.unwrap();

    TextBuffer::destroy(obj->buffer);

    heap_delete<TextEditLiteObj>(obj);
}

TextEditLiteResult TextEditLite::key(KeyValue value)
{
    TextEditLiteResult result = TEXT_EDIT_LITE_RESULT_NONE;
    const KeyCode code = value.code();
    const KeyMods mods = value.mods();

    if (mods == KEY_MOD_CONTROL_BIT && code == KEY_CODE_A)
    {
        mObj->select_all();
    }
    else if (KEY_CODE_SPACE <= code && code <= KEY_CODE_GRAVE_ACCENT)
    {
        // ascii printable
        char key = (char)code;

        if (KEY_CODE_A <= code && code <= KEY_CODE_Z && (mods & KEY_MOD_SHIFT_BIT) == 0)
            key += 32;
        else if (KEY_CODE_0 <= code && code <= KEY_CODE_9 && (mods & KEY_MOD_SHIFT_BIT))
            key = ")!@#$%^&*("[code - KEY_CODE_0];
        else if (KEY_CODE_LEFT_BRACKET <= code && code <= KEY_CODE_RIGHT_BRACKET && (mods & KEY_MOD_SHIFT_BIT))
            key = "{|}"[code - KEY_CODE_LEFT_BRACKET];
        else if (KEY_CODE_GRAVE_ACCENT == code && (mods & KEY_MOD_SHIFT_BIT))
            key = '~';
        else if (KEY_CODE_APOSTROPHE == code && (mods & KEY_MOD_SHIFT_BIT))
            key = '\"';
        else if (KEY_CODE_SEMICOLON == code && (mods & KEY_MOD_SHIFT_BIT))
            key = ':';
        else if (KEY_CODE_EQUAL == code && (mods & KEY_MOD_SHIFT_BIT))
            key = '+';
        else if (KEY_CODE_COMMA <= code && code <= KEY_CODE_SLASH && (mods & KEY_MOD_SHIFT_BIT))
            key = "<_>?"[code - KEY_CODE_COMMA];

        mObj->add(key);
        result = TEXT_EDIT_LITE_RESULT_CHANGED;
    }
    else if (KEY_CODE_KEYPAD_0 <= code && code <= KEY_CODE_KEYPAD_9)
    {
        char key = (code - KEY_CODE_KEYPAD_0) + '0';

        mObj->add(key);
        result = TEXT_EDIT_LITE_RESULT_CHANGED;
    }
    else if (code == KEY_CODE_BACKSPACE)
    {
        bool hasChanged = false;

        if (mods & KEY_MOD_CONTROL_BIT) // remove selection or previous word
        {
            hasChanged = mObj->remove_selection();

            if (!hasChanged)
            {
                // yeah this can definitely be done better
                std::string str = mObj->buffer.to_string();
                View view(str.data(), str.size());

                size_t pos = text_find_previous_word(view, mObj->cursor);
                hasChanged = mObj->remove_range(Range(pos, mObj->cursor - pos));
            }
        }
        else // remove selection or char
            hasChanged = mObj->remove();

        if (hasChanged)
            result = TEXT_EDIT_LITE_RESULT_CHANGED;
    }
    else if (code == KEY_CODE_DELETE)
    {
        if (mObj->deletion())
            result = TEXT_EDIT_LITE_RESULT_CHANGED;
    }
    else if (code == KEY_CODE_HOME)
    {
        mObj->cursor = 0;
    }
    else if (code == KEY_CODE_END)
    {
        mObj->cursor = mObj->buffer.size();
    }
    else if (code == KEY_CODE_LEFT)
    {
        if (!mObj->exit_selection_left())
            mObj->cursor_dec();
    }
    else if (code == KEY_CODE_RIGHT)
    {
        if (!mObj->exit_selection_right())
            mObj->cursor_inc();
    }
    else if (code == KEY_CODE_ENTER)
    {
        result = TEXT_EDIT_LITE_RESULT_SUBMITTED;
    }

    return result;
}

size_t TextEditLite::get_cursor()
{
    return mObj->cursor;
}

void TextEditLite::set_cursor(size_t pos)
{
    mObj->cursor = std::min(pos, mObj->buffer.size());
    mObj->selection = Range(0, 0);
}

Range TextEditLite::get_selection()
{
    return mObj->selection;
}

void TextEditLite::set_selection(Range selection)
{
    mObj->selection = Range::clamp_size(selection, mObj->buffer.size());
}

size_t TextEditLite::size()
{
    return mObj->buffer.size();
}

void TextEditLite::clear()
{
    mObj->buffer.clear();

    mObj->cursor = 0;
    mObj->selection = Range(0, 0);
}

void TextEditLite::set_string(View str)
{
    mObj->buffer.set_string(str);

    mObj->cursor = std::min(mObj->cursor, str.size);
    mObj->selection = Range(0, 0);
}

void TextEditLite::set_string(const std::string& str)
{
    set_string(View(str));
}

std::string TextEditLite::get_string()
{
    return mObj->buffer.to_string();
}

} // namespace LD