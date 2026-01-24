#include <Ludens/DSA/Vector.h>
#include <Ludens/Header/Assert.h>
#include <Ludens/Header/Bitwise.h>
#include <Ludens/Header/Types.h>
#include <Ludens/Media/Bitmap.h>
#include <Ludens/Memory/Memory.h>
#include <Ludens/Profiler/Profiler.h>
#include <Ludens/Serial/Compress.h>
#include <Ludens/System/FileSystem.h>

#include <cstring>

#include <stb/stb_image.h>
#include <stb/stb_image_write.h>

namespace LD {

using BitmapFlags = uint32_t;
enum BitmapFlagBit : BitmapFlags
{
    BITMAP_FLAG_USE_STB_FREE = LD_BIT(0),
    BITMAP_FLAG_USE_HEAP_FREE = LD_BIT(1),
};

struct BitmapObj
{
    uint32_t flags;
    uint32_t width;
    uint32_t height;
    BitmapFormat format;
    BitmapCompression compression;
    byte* data;
};

// clang-format off
struct
{
    uint32_t channels;
    uint32_t pixelSize;
} sBitmapFormatTable[] = {
    { 1, 1 },  // BITMAP_FORMAT_R8U
    { 3, 3 },  // BITMAP_FORMAT_RGB8U
    { 4, 4 },  // BITMAP_FORMAT_RGBA8U
    { 4, 16 }, // BITMAP_FORMAT_RGBA32F
};
// clang-format on

inline uint32_t get_channels_from_format(BitmapFormat format)
{
    return sBitmapFormatTable[(int)format].channels;
}

inline uint32_t get_pixel_size_from_format(BitmapFormat format)
{
    return sBitmapFormatTable[(int)format].pixelSize;
}

Bitmap Bitmap::create_from_data(uint32_t width, uint32_t height, BitmapFormat format, const void* data)
{
    LD_PROFILE_SCOPE;

    uint64_t pixelSize = (uint64_t)get_pixel_size_from_format(format);
    uint64_t dataSize = width * height * pixelSize;

    BitmapObj* obj = (BitmapObj*)heap_malloc(sizeof(BitmapObj) + dataSize, MEMORY_USAGE_MEDIA);
    obj->flags = 0;
    obj->width = width;
    obj->height = height;
    obj->format = format;
    obj->data = (byte*)(obj + 1);

    memcpy(obj->data, data, dataSize);

    return {obj};
}

Bitmap Bitmap::create_from_file_data(uint32_t fileSize, const void* fileData)
{
    LD_PROFILE_SCOPE;

    int x, y, ch;

    BitmapObj* obj = (BitmapObj*)heap_malloc(sizeof(BitmapObj), MEMORY_USAGE_MEDIA);
    obj->data = (byte*)stbi_load_from_memory((const stbi_uc*)fileData, (int)fileSize, &x, &y, &ch, STBI_rgb_alpha);

    if (!obj->data)
    {
        heap_free(obj);
        return {};
    }

    obj->format = BITMAP_FORMAT_RGBA8U;
    obj->flags = BITMAP_FLAG_USE_STB_FREE;
    obj->width = (uint32_t)x;
    obj->height = (uint32_t)y;

    return Bitmap(obj);
}

Bitmap Bitmap::create_from_path(const char* path, bool isF32)
{
    LD_PROFILE_SCOPE;

    BitmapObj* obj = (BitmapObj*)heap_malloc(sizeof(BitmapObj), MEMORY_USAGE_MEDIA);

    int x, y, ch;

    if (isF32)
    {
        obj->data = (byte*)stbi_loadf(path, &x, &y, &ch, STBI_rgb_alpha);
        obj->format = BITMAP_FORMAT_RGBA32F;
    }
    else
    {
        obj->data = (byte*)stbi_load(path, &x, &y, &ch, STBI_rgb_alpha);
        obj->format = BITMAP_FORMAT_RGBA8U;
    }

    if (!obj->data)
        return {};

    obj->flags = BITMAP_FLAG_USE_STB_FREE;
    obj->width = (uint32_t)x;
    obj->height = (uint32_t)y;

    return {obj};
}

Bitmap Bitmap::create_cubemap_from_paths(const char** paths)
{
    LD_PROFILE_SCOPE;

    BitmapObj* obj = (BitmapObj*)heap_malloc(sizeof(BitmapObj), MEMORY_USAGE_MEDIA);
    uint32_t size = 0;
    uint32_t layerSize = 0;
    byte* dst = nullptr;
    byte* tmp = nullptr;

    for (int i = 0; i < 6; i++)
    {
        int x, y, ch;
        tmp = (byte*)stbi_load(paths[i], &x, &y, &ch, STBI_rgb_alpha);

        if (!tmp)
        {
            printf("cubemap face not found: %s\n", paths[i]);
            goto failure;
        }

        if (x != y)
        {
            printf("cubemap face %d is not a cube: %dx%d\n", i, x, y);
            goto failure;
        }

        if (i == 0)
        {
            size = (uint32_t)x;
            layerSize = size * size * 4;
            dst = obj->data = (byte*)heap_malloc(6 * layerSize, MEMORY_USAGE_MEDIA);
        }
        else if (x != (int)size || y != (int)size)
        {
            printf("cubemap faces vary in size, expected %dx%d for face %d, found %dx%d\n", (int)size, (int)size, i, x, y);
            goto failure;
        }

        // copy into contiguous memory
        memcpy(dst, tmp, layerSize);
        stbi_image_free(tmp);

        dst += layerSize;
    }

    obj->flags = BITMAP_FLAG_USE_HEAP_FREE;
    obj->width = size;
    obj->height = size;
    obj->format = BITMAP_FORMAT_RGBA8U;

    return {obj};

failure:

    if (tmp)
        stbi_image_free(tmp);

    if (obj)
    {
        if (obj->data)
            heap_free(obj->data);
        heap_free(obj);
    }

    return {};
}

Bitmap Bitmap::create_cubemap_from_data(uint32_t size, const void* faceData[6])
{
    const uint32_t layerSize = size * size * 4;
    BitmapObj* obj = (BitmapObj*)heap_malloc(sizeof(BitmapObj), MEMORY_USAGE_MEDIA);
    obj->data = (byte*)heap_malloc(6 * layerSize, MEMORY_USAGE_MEDIA);
    obj->flags = BITMAP_FLAG_USE_HEAP_FREE;
    obj->width = size;
    obj->height = size;
    obj->format = BITMAP_FORMAT_RGBA8U;

    for (int i = 0; i < 6; i++)
    {
        // copy into contiguous memory
        memcpy(obj->data + i * layerSize, faceData[i], layerSize);
    }

    return Bitmap(obj);
}

Bitmap Bitmap::create_cubemap_from_file_data(uint32_t fileSizes[6], const void* fileData[6])
{
    int width, height, ch;

    uint32_t faceSize = 0;
    void* faceData[6];

    faceData[0] = stbi_load_from_memory((const stbi_uc*)fileData[0], (int)fileSizes[0], &width, &height, &ch, STBI_rgb_alpha);
    if (width != height)
        return {};

    faceSize = (uint32_t)width;

    for (int i = 1; i < 6; i++)
    {
        faceData[i] = stbi_load_from_memory((const stbi_uc*)fileData[i], (int)fileSizes[i], &width, &height, &ch, STBI_rgb_alpha);
        if (width != (int)faceSize || height != (int)faceSize)
            return {}; // TODO: fix leak
    }

    Bitmap cubemap = create_cubemap_from_data(faceSize, (const void**)faceData);

    for (int i = 0; i < 6; i++)
        stbi_image_free((void*)faceData[i]);

    return cubemap;
}

void Bitmap::destroy(Bitmap bitmap)
{
    LD_PROFILE_SCOPE;

    BitmapObj* obj = (BitmapObj*)bitmap;

    if (obj->data && (obj->flags & BITMAP_FLAG_USE_STB_FREE))
        stbi_image_free(obj->data);
    else if (obj->data && (obj->flags & BITMAP_FLAG_USE_HEAP_FREE))
        heap_free(obj->data);

    heap_free(obj);
}

bool Bitmap::serialize(Serializer& serial, const Bitmap& bitmap)
{
    LD_PROFILE_SCOPE;

    const BitmapObj* obj = bitmap;
    const uint32_t pixelSize = get_pixel_size_from_format(obj->format);

    serial.write_u32(obj->width);
    serial.write_u32(obj->height);
    serial.write_u32((uint32_t)obj->format);
    serial.write_u32((uint32_t)obj->compression);

    LD_ASSERT(obj->compression == BITMAP_COMPRESSION_LZ4);

    size_t dataSize = obj->width * obj->height * pixelSize;
    size_t sizeBound = lz4_compress_bound(dataSize);
    Vector<byte> compressed(sizeBound);
    size_t cmpSize = lz4_compress(compressed.data(), compressed.size(), obj->data, dataSize);
    compressed.resize(cmpSize);

    serial.write_u64((uint64_t)compressed.size());
    serial.write(compressed.data(), compressed.size());

    return true;
}

bool Bitmap::deserialize(Deserializer& serial, Bitmap& bitmap)
{
    LD_PROFILE_SCOPE;

    uint32_t width, height, formatU32, compressionU32;
    serial.read_u32(width);
    serial.read_u32(height);
    serial.read_u32(formatU32);
    serial.read_u32(compressionU32);

    const BitmapFormat format = (BitmapFormat)formatU32;
    const uint32_t pixelSize = get_pixel_size_from_format(format);

    uint64_t lz4BlockSize;
    serial.read_u64(lz4BlockSize);

    const byte* lz4BlockData = serial.view_now();
    serial.advance(lz4BlockSize);

    size_t dataSize = width * height * pixelSize;
    Vector<byte> pixels(dataSize);
    lz4_decompress(pixels.data(), pixels.size(), lz4BlockData, lz4BlockSize);

    bitmap = Bitmap::create_from_data(width, height, format, pixels.data());

    return true;
}

void Bitmap::flipy()
{
    LD_PROFILE_SCOPE;

    byte* data = mObj->data;
    byte tmp[2048];
    const uint32_t width = mObj->width;
    const uint32_t height = mObj->height;
    const uint32_t pixelSize = get_pixel_size_from_format(mObj->format); // currently only supports one byte per pixel channel
    const uint32_t bytesPerRow = pixelSize * width;

    for (uint32_t row = 0; row < height / 2; row++)
    {
        uint8_t* row0 = data + row * bytesPerRow;
        uint8_t* row1 = data + (height - row - 1) * bytesPerRow;
        uint32_t bytesLeft = bytesPerRow;

        while (bytesLeft)
        {
            size_t bytesCopied = (bytesLeft < sizeof(tmp)) ? bytesLeft : sizeof(tmp);
            memcpy(tmp, row0, bytesCopied);
            memcpy(row0, row1, bytesCopied);
            memcpy(row1, tmp, bytesCopied);
            row0 += bytesCopied;
            row1 += bytesCopied;
            bytesLeft -= bytesCopied;
        }
    }
}

BitmapView Bitmap::view() const
{
    return {
        .width = mObj->width,
        .height = mObj->height,
        .format = mObj->format,
        .data = (const char*)mObj->data,
    };
}

uint32_t Bitmap::width() const
{
    return mObj->width;
}

uint32_t Bitmap::height() const
{
    return mObj->height;
}

uint32_t Bitmap::pixel_size() const
{
    return get_pixel_size_from_format(mObj->format);
}

BitmapFormat Bitmap::format() const
{
    return mObj->format;
}

byte* Bitmap::data()
{
    return mObj->data;
}

const byte* Bitmap::data() const
{
    return (const byte*)mObj->data;
}

void Bitmap::set_compression(BitmapCompression compression)
{
    mObj->compression = compression;
}

bool Bitmap::save_to_disk(const BitmapView& view, const char* c_path)
{
    LD_PROFILE_SCOPE;

    FS::Path path(c_path);
    std::string ext = path.extension().string();

    if (ext == ".png")
    {
        int ch = (int)get_channels_from_format(view.format);
        int success = stbi_write_png(c_path, view.width, view.height, ch, view.data, 0);
        if (!success)
        {
            printf("save_bitmap_to_disk: stbi_write_png error\n");
            return false;
        }
        printf("save_bitmap_to_disk: %s\n", c_path);
        return true;
    }
    else
    {
        printf("save_bitmap_to_disk: unsupported extension: %s\n", ext.c_str());
        return false;
    }

    return false;
}

bool Bitmap::compute_mse(const BitmapView& lhs, const BitmapView& rhs, double& outMSE)
{
    LD_PROFILE_SCOPE;

    if (lhs.format != rhs.format || lhs.width != rhs.width || lhs.height != rhs.height)
    {
        outMSE = -1.0;
        return false;
    }

    if (lhs.data == rhs.data)
    {
        outMSE = 0.0;
        return true;
    }

    outMSE = 0.0;

    const uint32_t W = lhs.width;
    const uint32_t H = lhs.height;
    const uint32_t CH = get_channels_from_format(lhs.format);
    const uint8_t* ldata = (const uint8_t*)lhs.data;
    const uint8_t* rdata = (const uint8_t*)rhs.data;

    for (uint32_t h = 0; h < H; h++)
    {
        for (uint32_t w = 0; w < W; w++)
        {
            uint32_t texelIdx = h * W + w;
            double texelSE = 0.0;

            for (uint32_t c = 0; c < CH; c++)
            {
                double err = (ldata[texelIdx * CH + c] / 255.0) - (rdata[texelIdx * CH + c] / 255.0);
                texelSE += err * err;
            }

            outMSE += texelSE;
        }
    }

    outMSE /= static_cast<double>(W * H * CH);
    return true;
}

} // namespace LD
