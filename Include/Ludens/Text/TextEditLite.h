#pragma once

#include <Ludens/Header/Handle.h>
#include <Ludens/Header/KeyValue.h>
#include <Ludens/Header/View.h>

#include <cstddef>
#include <string>

namespace LD {

enum TextEditLiteResult
{
    TEXT_EDIT_LITE_RESULT_NONE = 0,
    TEXT_EDIT_LITE_RESULT_CHANGED,
    TEXT_EDIT_LITE_RESULT_SUBMITTED,
};

/// @brief Lightweight text editor.
struct TextEditLite : Handle<struct TextEditLiteObj>
{
    static TextEditLite create();
    static void destroy(TextEditLite editor);

    /// @brief Process key.
    TextEditLiteResult key(KeyValue value);

    /// @brief Get cursor byte offset.
    size_t get_cursor();

    /// @brief Set cursor byte offset.
    void set_cursor(size_t pos);

    /// @brief Get byte size.
    size_t size();

    void clear();
    void set_string(View str);
    void set_string(const std::string& str);
    std::string get_string();
};

} // namespace LD