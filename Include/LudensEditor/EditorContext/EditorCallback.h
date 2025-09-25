#pragma once

#include <Ludens/Asset/Asset.h>

namespace LD {

/// @brief Select an asset in project.
typedef void (*ECBSelectAssetFn)(AssetType type, AUID currentID, void* user);

} // namespace LD