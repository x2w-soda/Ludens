#pragma once

namespace LD {

/// @brief Blob asset implementation.
struct BlobAssetObj : AssetObj
{
    void* data;
    void* fileData;
    uint64_t dataSize;

    static void load(void* assetLoadJob);
    static void unload(AssetObj* base);
};

} // namespace LD