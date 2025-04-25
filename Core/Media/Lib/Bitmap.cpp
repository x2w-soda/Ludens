#include <Ludens/Media/Bitmap.h>
#include <Ludens/System/Memory.h>
#include <cstring>
#include <filesystem>
#include <stb/stb_image.h>
#include <stb/stb_image_write.h>

namespace LD {

struct BitmapObj
{
    uint32_t width;
    uint32_t height;
    BitmapChannel channel;
    char* data;
    bool fromStb;
};

Bitmap Bitmap::create_from_data(uint32_t width, uint32_t height, BitmapChannel channel, const void* data)
{
    uint64_t dataSize = width * height * channel;

    BitmapObj* obj = (BitmapObj*)heap_malloc(sizeof(BitmapObj) + dataSize, MEMORY_USAGE_MEDIA);
    obj->width = width;
    obj->height = height;
    obj->channel = channel;
    obj->data = (char*)(obj + 1);
    obj->fromStb = false;

    memcpy(obj->data, data, dataSize);

    return {obj};
}

Bitmap Bitmap::create_from_path(const char* path)
{
    BitmapObj* obj = (BitmapObj*)heap_malloc(sizeof(BitmapObj), MEMORY_USAGE_MEDIA);

    int x, y, ch;
    obj->data = (char*)stbi_load(path, &x, &y, &ch, STBI_rgb_alpha);

    if (!obj->data)
        return {};

    obj->width = (uint32_t)x;
    obj->height = (uint32_t)y;
    obj->channel = BITMAP_CHANNEL_RGBA;
    obj->fromStb = true;

    return {obj};
}

void Bitmap::destroy(Bitmap bitmap)
{
    BitmapObj* obj = (BitmapObj*)bitmap;

    if (obj->fromStb)
        stbi_image_free(obj->data);

    heap_free(obj);
}

BitmapView Bitmap::view() const
{
    return {
        .width = mObj->width,
        .height = mObj->height,
        .channel = mObj->channel,
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

BitmapChannel Bitmap::channel() const
{
    return mObj->channel;
}

char* Bitmap::data()
{
    return mObj->data;
}

const char* Bitmap::data() const
{
    return (const char*)mObj->data;
}

bool save_bitmap_to_disk(const BitmapView& view, const char* c_path)
{
    namespace fs = std::filesystem;

    fs::path path(c_path);
    std::string ext = path.extension().string();

    if (ext == ".png")
    {
        int success = stbi_write_png(c_path, view.width, view.height, view.channel, view.data, 0);
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

} // namespace LD