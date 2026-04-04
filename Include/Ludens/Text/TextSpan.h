#pragma once

#include <Ludens/Header/Bitwise.h>
#include <Ludens/Header/Color.h>
#include <Ludens/Header/Range.h>
#include <Ludens/Header/View.h>

namespace LD {

enum TextSpanFont : uint8_t
{
    TEXT_SPAN_FONT_REGULAR,
    TEXT_SPAN_FONT_MONOSPACE,
};

using TextSpanFlagBits = uint8_t;
enum TextSpanFlagBit : TextSpanFlagBits
{
    TEXT_SPAN_UNDERLINE_BIT = LD_BIT(0),
    TEXT_SPAN_STRIKETHROUGH_BIT = LD_BIT(1),
};

/// @brief A sequence of text that should be rendered with the same font, color, and attributes.
/// @note This is rendering intent that is interpreted later by the UI module.
struct TextSpan
{
    Range range;            // UTF-8 byte range
    Color fgColor;          // foreground text color
    TextSpanFlagBits flags; // rendering hints
    TextSpanFont font;      // desired font
};

} // namespace LD