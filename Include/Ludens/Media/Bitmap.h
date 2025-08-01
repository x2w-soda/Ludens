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

enum BitmapCompression
{
    /// @brief Use LZ4 for serialization
    BITMAP_COMPRESSION_LZ4 = 0,
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
    static Bitmap create_from_path(const char* path, bool isF32 = false);

    /// @brief create 6 layered bitmap from 6 paths to each face
    /// @param paths an array of 6 paths
    static Bitmap create_cubemap_from_paths(const char** paths);

    /// @brief destroy bitmap
    static void destroy(Bitmap bitmap);

    /// @brief serialize a bitmap to binary data
    static void serialize(Serializer& serializer, const Bitmap& bitmap);

    /// @brief create bitmap from binary data
    static void deserialize(Serializer& serializer, Bitmap& bitmap);

    /// @brief flip image vertically along the Y axis
    void flipy();

    /// @brief get a const view of bitmap
    BitmapView view() const;

    /// @brief get bitmap width in pixels
    uint32_t width() const;

    /// @brief get bitmap height in pixels
    uint32_t height() const;

    /// @brief get the byte size of one pixel
    uint32_t pixel_size() const;

    /// @brief get bitmap channels
    BitmapChannel channel() const;

    /// @brief view of bitmap as byte stream
    byte* data();

    /// @brief const view of bitmap as byte stream
    const byte* data() const;

    /// @brief Set bitmap compression method for serialize().
    void set_compression(BitmapCompression compression);
};

bool save_bitmap_to_disk(const BitmapView& view, const char* path);

} // namespace LD