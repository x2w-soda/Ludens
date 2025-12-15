#pragma once

#include <Ludens/Asset/Asset.h>
#include <Ludens/JobSystem/JobSystem.h>
#include <filesystem>

namespace LD {

/// @brief Lua script asset handle.
struct LuaScriptAsset : Asset
{
    /// @brief Unload asset from RAM.
    void unload();

    /// @brief Get Lua script source string.
    const char* get_source();

    /// @brief Set Lua script source string.
    /// @note This only modifies the asset in RAM.
    void set_source(const char* src, size_t len);
};

} // namespace LD