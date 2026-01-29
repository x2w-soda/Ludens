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
template <typename T>
struct TextBufferObj
{
    GapBuffer<T> gapBuffer;
};

template <typename T>
TextBuffer<T> TextBuffer<T>::create()
{
    TextBufferObj<T>* obj = heap_new<TextBufferObj<T>>(MEMORY_USAGE_TEXT_EDIT);

    return TextBuffer<T>(obj);
}

template <typename T>
void TextBuffer<T>::destroy(TextBuffer<T> buf)
{
    TextBufferObj<T>* obj = buf.unwrap();

    heap_delete<TextBufferObj<T>>(obj);
}

template <typename T>
void TextBuffer<T>::set_string(View view)
{
    TextBufferObj<T>* obj = this->mObj;

    obj->gapBuffer.clear();
    obj->gapBuffer.insert(0, view);
}

template <typename T>
void TextBuffer<T>::set_string(const char* cstr)
{
    TextBufferObj<T>* obj = this->mObj;

    obj->gapBuffer.clear();

    if (!cstr)
        return;

    obj->gapBuffer.insert(0, cstr);
}

template <typename T>
std::basic_string<T> TextBuffer<T>::to_string()
{
    TextBufferObj<T>* obj = this->mObj;

    return obj->gapBuffer.to_string();
}

template <typename T>
bool TextBuffer<T>::empty()
{
    TextBufferObj<T>* obj = this->mObj;

    return obj->gapBuffer.size() == 0;
}

template <typename T>
void TextBuffer<T>::push_back(T ch)
{
    TextBufferObj<T>* obj = this->mObj;

    obj->gapBuffer.insert(obj->gapBuffer.size(), ch);
}

template <typename T>
void TextBuffer<T>::pop_back()
{
    TextBufferObj<T>* obj = this->mObj;

    if (obj->gapBuffer.size() == 0)
        return;

    obj->gapBuffer.erase(obj->gapBuffer.size() - 1, 1);
}

//
// Explicit instantiations for ASCII and Unicode text buffers.
// Expect an overall 4x memory footprint difference between the two.
//

template struct TextBuffer<char>;
template struct TextBuffer<uint32_t>;

} // namespace LD