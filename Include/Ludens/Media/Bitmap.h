#pragma once

#include <Ludens/Header/Handle.h>
#include <Ludens/Serial/Serial.h>
#include <cstdint>

namespace LD {

enum BitmapFormat
{
    /// @brief Grayscale, uint8_t in RAM
    BITMAP_FORMAT_R8U = 0,

    /// @brief RGB Colored, uint8_t channels in RAM
    BITMAP_FORMAT_RGB8U,

    /// @brief RGB with Alpha, uint8_t channels in RAM
    BITMAP_FORMAT_RGBA8U,

    /// @brief RGB with Alpha, float channels in RAM
    BITMAP_FORMAT_RGBA32F,
};

enum BitmapCompression
{
    /// @brief Use LZ4 for serialization
    BITMAP_COMPRESSION_LZ4 = 0,
};

/// @brief Read only view of bitmap data.
struct BitmapView
{
    const uint32_t width;
    const uint32_t height;
    BitmapFormat format;
    const char* data;
};

struct Bitmap : Handle<struct BitmapObj>
{
    /// @brief create bitmap from copying existing data,
    ///        the memory is released after calling destroy
    static Bitmap create_from_data(uint32_t width, uint32_t height, BitmapFormat format, const void* data);

    /// @brief create bitmap from file on disk
    static Bitmap create_from_path(const char* path, bool isF32 = false);

    /// @brief create 6 layered bitmap from 6 paths to each face
    /// @param paths an array of 6 paths
    static Bitmap create_cubemap_from_paths(const char** paths);

    /// @brief Create 6 layered bitmap from 6-face data.
    /// @param size Size of each face.
    /// @param faceData An array of cubemap faces, each face contains RGBA8 pixels
    static Bitmap create_cubemap_from_data(uint32_t size, const void* faceData[6]);

    static Bitmap create_cubemap_from_file_data(uint32_t fileSizes[6], const void* fileData[6]);

    /// @brief destroy bitmap
    static void destroy(Bitmap bitmap);

    /// @brief serialize a bitmap to binary data
    static bool serialize(Serializer& serializer, const Bitmap& bitmap);

    /// @brief create bitmap from binary data
    static bool deserialize(Deserializer& serial, Bitmap& bitmap);

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

    /// @brief get bitmap format for underlying data.
    BitmapFormat format() const;

    /// @brief view of bitmap as byte stream
    byte* data();

    /// @brief const view of bitmap as byte stream
    const byte* data() const;

    /// @brief Set bitmap compression method for serialize().
    void set_compression(BitmapCompression compression);

    /// @brief Save bitmap to disk.
    static bool save_to_disk(const BitmapView& view, const char* path);

    /// @brief Compute mean squared error between two bitmaps of equal dimension.
    /// @return True if MSE could be computed.
    static bool compute_mse(const BitmapView& lhs, const BitmapView& rhs, double& outMSE);
};

} // namespace LD