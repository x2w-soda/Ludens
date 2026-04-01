#pragma once

#include <Ludens/Asset/Asset.h>

#include <cstddef>

namespace LD {

/// @brief Blob asset handle. The engine makes no assumptions about the binary contents.
struct BlobAsset : Asset
{
    /// @brief Get blob data.
    void* get_data(size_t& dataSize);
};

} // namespace LD