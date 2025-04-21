#pragma once

#include <Ludens/Header/Handle.h>
#include <cstdint>

namespace LD {

enum BitmapChannel
{
    BITMAP_CHANNEL_R = 1,
    BITMAP_CHANNEL_RGB = 3,
    BITMAP_CHANNEL_RGBA = 4,
};

/// read only view of bitmap data
struct BitmapView
{
    const uint32_t width;
    const uint32_t height;
    BitmapChannel channel;
    const char* data;
};

struct Bitmap : Handle<struct BitmapObj>
{
    /// @brief create bitmap from copying existing data,
    ///        the memory is released after calling destroy
    static Bitmap create_from_data(uint32_t width, uint32_t height, BitmapChannel channel, const void* data);

    /// @brief create bitmap from file on disk
    static Bitmap create_from_path(const char* path);

    /// @brief destroy bitmap
    static void destroy(Bitmap bitmap);

    BitmapView view() const;

    uint32_t width() const;
    uint32_t height() const;
    BitmapChannel channel() const;

    char* data();
    const char* data() const;
};

} // namespace LD