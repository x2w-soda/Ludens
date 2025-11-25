#pragma once

#include <Ludens/Header/Handle.h>
#include <string>

namespace LD {

template <typename T>
struct TextBufferObj;

/// @brief A buffer of text subject to editing.
template <typename T>
struct TextBuffer : Handle<TextBufferObj<T>>
{
    /// @brief Create a text buffer
    static TextBuffer create();

    /// @brief Destroy a text buffer
    static void destroy(TextBuffer buf);

    /// @brief Set text buffer content to C string.
    void set_string(const char* cstr);

    /// @brief Cast to STL string.
    std::basic_string<T> to_string();

    /// @brief Check if text buffer is empty.
    bool empty();

    /// @brief Append a char at the end of buffer.
    void push_back(T ch);

    /// @brief Erase the char at the end of buffer, safe even if buffer is empty.
    void pop_back();
};

} // namespace LD