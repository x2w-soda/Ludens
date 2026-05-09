#pragma once

#include <Ludens/DSA/String.h>
#include <Ludens/Header/Handle.h>

namespace LD {

/// @brief A buffer of text subject to editing.
struct TextBuffer : Handle<struct TextBufferObj>
{
    /// @brief Create a text buffer
    static TextBuffer create();

    /// @brief Destroy a text buffer
    static void destroy(TextBuffer buf);

    /// @brief Set text buffer content to view content.
    void set_string(View view);

    /// @brief Set text buffer content to C string.
    void set_string(const char* cstr);

    /// @brief Set text buffer content
    inline void set_string(const String& str) { set_string(str.c_str()); }

    /// @brief Cast to STL string.
    String to_string();

    /// @brief Content byte size.
    size_t size();

    /// @brief Check if text buffer is empty.
    inline bool empty() { return size() == 0; }

    /// @brief Clears text buffer content.
    void clear();

    /// @brief Insert a char at byte offset.
    void insert(size_t pos, char ch);

    /// @brief Delete a char at byte offset.
    void erase(size_t pos);

    /// @brief Append a char at the end of buffer.
    void push_back(char ch);

    /// @brief Erase the char at the end of buffer, safe even if buffer is empty.
    void pop_back();
};

} // namespace LD