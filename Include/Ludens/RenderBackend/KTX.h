#pragma once

#include <Ludens/RenderBackend/RBackend.h>
#include <Ludens/System/FileSystem.h>

struct ktxTexture2;

namespace LD {

/// @brief KTX texture creation info.
struct KTXTextureInfo
{
    const void* data; // raw pixel data, tightly packed rows.
    size_t dataSize;  // free sanity check, compared against pixel format and image dimensions
    RFormat format;
    uint32_t layers;
    uint32_t width;
    uint32_t height;
};

/// @brief KTX texture object.
struct KTXTexture
{
    ktxTexture2* handle = nullptr;
    RFormat format;
    uint32_t layers;
    uint32_t width;
    uint32_t height;

    /// @brief Create KTX texture object.
    /// @return True on success.
    static bool create(const KTXTextureInfo& info, KTXTexture& texture);

    /// @brief Destroy KTX texture object.
    static void destroy(KTXTexture texture);

    /// @brief Write to disk.
    bool write_to_disk(const FS::Path& path);
};

} // namespace LD