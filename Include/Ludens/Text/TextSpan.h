#pragma once

#include <Ludens/Header/Bitwise.h>
#include <Ludens/Header/Color.h>
#include <Ludens/Header/View.h>

namespace LD {

using TextSpanFlagBits = uint32_t;
enum TextSpanFlagBit : TextSpanFlagBits
{
    TEXT_SPAN_UNDERLINE_BIT = LD_BIT(0),
    TEXT_SPAN_STRIKETHROUGH_BIT = LD_BIT(1),
};

/// @brief A sequence of text that should be rendered with the same font, color and attributes.
struct TextSpan
{
    View text;              // non-owning view into UTF-8 text
    Color fgColor;          // foreground text color
    TextSpanFlagBits flags; // rendering hints
};

} // namespace LD