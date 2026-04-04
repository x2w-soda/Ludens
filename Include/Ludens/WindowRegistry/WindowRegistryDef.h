#pragma once

#include <cstdint>

namespace LD {

using WindowID = uint32_t;

enum CursorType
{
    CURSOR_TYPE_DEFAULT = 0, /// Default system cursor state, usually arrow shape
    CURSOR_TYPE_IBEAM,       /// I-Beam cursor shape, hints at text input
    CURSOR_TYPE_CROSSHAIR,   /// Crosshair cursor shape
    CURSOR_TYPE_HAND,        /// Hand cursor shape
    CURSOR_TYPE_HRESIZE,     /// Horizontal resize cursor shape
    CURSOR_TYPE_VRESIZE,     /// Vertical resize cursor shape
    CURSOR_TYPE_ENUM_COUNT,
};

} // namespace LD