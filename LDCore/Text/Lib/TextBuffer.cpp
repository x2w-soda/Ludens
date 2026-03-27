#include <Ludens/DSA/GapBuffer.h>
#include <Ludens/DSA/String.h>
#include <Ludens/Header/Assert.h>
#include <Ludens/Header/Types.h>
#include <Ludens/Memory/Memory.h>
#include <Ludens/Text/TextBuffer.h>
#include <cstdint>

namespace LD {

/// @brief Text buffer implementation. Currently this is just a gap buffer
///        for general purpose text, we can implement other data structures
///        for more serious text editing later.
struct TextBufferObj
{
    GapBuffer<char> gapBuffer;
};

TextBuffer TextBuffer::create()
{
    TextBufferObj* obj = heap_new<TextBufferObj>(MEMORY_USAGE_TEXT_EDIT);

    return TextBuffer(obj);
}

void TextBuffer::destroy(TextBuffer buf)
{
    TextBufferObj* obj = buf.unwrap();

    heap_delete<TextBufferObj>(obj);
}

void TextBuffer::set_string(View view)
{
    mObj->gapBuffer.clear();
    mObj->gapBuffer.insert(0, view);
}

void TextBuffer::set_string(const char* cstr)
{
    mObj->gapBuffer.clear();

    if (!cstr)
        return;

    mObj->gapBuffer.insert(0, cstr);
}

std::string TextBuffer::to_string()
{
    return mObj->gapBuffer.to_string();
}

size_t TextBuffer::size()
{
    return mObj->gapBuffer.size();
}

void TextBuffer::clear()
{
    return mObj->gapBuffer.clear();
}

void TextBuffer::insert(size_t pos, char ch)
{
    mObj->gapBuffer.insert(pos, ch);
}

void TextBuffer::erase(size_t pos)
{
    mObj->gapBuffer.erase(pos, 1);
}

void TextBuffer::push_back(char ch)
{
    mObj->gapBuffer.insert(mObj->gapBuffer.size(), ch);
}

void TextBuffer::pop_back()
{
    if (mObj->gapBuffer.size() == 0)
        return;

    mObj->gapBuffer.erase(mObj->gapBuffer.size() - 1, 1);
}

} // namespace LD