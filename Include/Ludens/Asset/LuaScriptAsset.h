#pragma once

#include <Ludens/Asset/Asset.h>
#include <Ludens/Header/Handle.h>
#include <Ludens/JobSystem/JobSystem.h>
#include <filesystem>

namespace LD {

/// @brief Lua script asset handle.
struct LuaScriptAsset : Handle<struct LuaScriptAssetObj>
{
    /// @brief Get asset ID.
    AUID auid() const;

    /// @brief Unload asset from RAM.
    void unload();

    /// @brief Get Lua script source string.
    const char* get_source();
};

/// @brief Loads LuaScriptAsset from disk
class LuaScriptAssetLoadJob
{
public:
    LuaScriptAsset asset;           /// subject asset handle
    std::filesystem::path loadPath; /// path to load the imported format

    /// @brief Submit to job system. Address of this job instance must not
    ///        change until the worker thread completes execution.
    void submit();

private:
    static void execute(void*);

    JobHeader mHeader;
};

} // namespace LD