#pragma once

#include "Core/Header/Include/Types.h"
#include "Core/Math/Include/Vec4.h"

namespace LD {

/// utility class for 32-bit hexadecimal values
struct Hex
{
    float r;
    float g;
    float b;
    float a;

    Hex(float r, float g, float b, float a) : r(r), g(g), b(b), a(a)
    {
    }

    Hex(u32 hex)
        : r(((hex >> 24) & 0xFF) / 255.0f), g(((hex >> 16) & 0xFF) / 255.0f), b(((hex >> 8) & 0xFF) / 255.0f),
          a((hex & 0xFF) / 255.0f)
    {
    }

    TVec3<float> RGB() const
    {
        return { r, g, b };
    }

    operator TVec4<float>() const
    {
        return TVec4<float>{ r, g, b, a };
    }

    inline Hex Complement() const
    {
        return Hex{ 1.0f - r, 1.0f - g, 1.0f - b, a };
    }
};

} // namespace LD