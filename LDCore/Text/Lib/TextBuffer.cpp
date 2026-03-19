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
    TextBufferObj* obj = this->mObj;

    obj->gapBuffer.clear();
    obj->gapBuffer.insert(0, view);
}

void TextBuffer::set_string(const char* cstr)
{
    TextBufferObj* obj = this->mObj;

    obj->gapBuffer.clear();

    if (!cstr)
        return;

    obj->gapBuffer.insert(0, cstr);
}


std::string TextBuffer::to_string()
{
    TextBufferObj* obj = this->mObj;

    return obj->gapBuffer.to_string();
}


void TextBuffer::clear()
{
    TextBufferObj* obj = this->mObj;

    return obj->gapBuffer.clear();
}


bool TextBuffer::empty()
{
    TextBufferObj* obj = this->mObj;

    return obj->gapBuffer.size() == 0;
}


void TextBuffer::push_back(char ch)
{
    TextBufferObj* obj = this->mObj;

    obj->gapBuffer.insert(obj->gapBuffer.size(), ch);
}


void TextBuffer::pop_back()
{
    TextBufferObj* obj = this->mObj;

    if (obj->gapBuffer.size() == 0)
        return;

    obj->gapBuffer.erase(obj->gapBuffer.size() - 1, 1);
}

} // namespace LD