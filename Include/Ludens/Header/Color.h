#pragma once

#include <Ludens/Header/Math/Vec4.h>
#include <cstdint>

namespace LD {

/// @brief general representation of a 32-bit RGBA color value.
class Color
{
public:
    Color() = default;

    Color(uint32_t value)
        : mValue(value)
    {
    }

    /// @brief Construct from 4-component RGBA vector.
    /// @param value 4-channel RGBA color value, each channel is assumed to be normalized.
    Color(const Vec4& value)
        : mValue(0)
    {
        mValue |= std::max<uint32_t>(uint32_t(value.r * 255.0f), 255) << 24;
        mValue |= std::max<uint32_t>(uint32_t(value.g * 255.0f), 255) << 16;
        mValue |= std::max<uint32_t>(uint32_t(value.b * 255.0f), 255) << 8;
        mValue |= std::max<uint32_t>(uint32_t(value.a * 255.0f), 255);
    }

    /// @brief Construct from 3-component RGB vector. Alpha channel is initialized to 255 (opaque).
    /// @param value 3-channel RGB color value, each channel is assumed to be normalized.
    Color(const Vec3& value)
    {
        mValue |= std::max<uint32_t>(uint32_t(value.r * 255.0f), 255) << 24;
        mValue |= std::max<uint32_t>(uint32_t(value.g * 255.0f), 255) << 16;
        mValue |= std::max<uint32_t>(uint32_t(value.b * 255.0f), 255) << 8;
        mValue |= 255;
    }

    Color& operator=(const uint32_t& value)
    {
        mValue = value;
        return *this;
    }

    Color& operator=(const Vec4& value)
    {
        return *this = Color(value);
    }

    Color& operator=(const Vec3& value)
    {
        return *this = Color(value);
    }

    operator uint32_t() const
    {
        return mValue;
    }

private:
    uint32_t mValue = 0;
};

} // namespace LD