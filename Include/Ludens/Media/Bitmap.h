#pragma once

#include <Ludens/Header/Handle.h>
#include <Ludens/Serial/Serial.h>
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

    /// @brief create 6 layered bitmap from 6 paths to each face
    /// @param paths an array of 6 paths
    static Bitmap create_cubemap_from_paths(const char** paths);

    /// @brief destroy bitmap
    static void destroy(Bitmap bitmap);

    /// @brief serialize a bitmap to binary data
    static void serialize(Serializer& serializer, const Bitmap& bitmap);

    /// @brief create bitmap from binary data
    static void deserialize(Serializer& serializer, Bitmap& bitmap);

    BitmapView view() const;

    uint32_t width() const;
    uint32_t height() const;
    BitmapChannel channel() const;

    byte* data();
    const byte* data() const;
};

bool save_bitmap_to_disk(const BitmapView& view, const char* path);

} // namespace LD