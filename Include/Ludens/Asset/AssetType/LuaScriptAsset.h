#pragma once

#include <Ludens/Asset/Asset.h>
#include <Ludens/JobSystem/JobSystem.h>
#include <Ludens/System/FileSystem.h>

namespace LD {

enum LuaScriptDomain
{
    LUA_SCRIPT_DOMAIN_GENERAL = 0,

    /// @brief Lua script returns a table for instantiating component scripts.
    LUA_SCRIPT_DOMAIN_COMPONENT = 1,
};

/// @brief Lua script asset handle.
struct LuaScriptAsset : Asset
{
    /// @brief Unload asset from RAM.
    void unload();

    /// @brief Get relative path to Lua script file.
    FS::Path get_source_path();

    /// @brief Get Lua script source string.
    const char* get_source();

    /// @brief Set Lua script source string.
    /// @note This only modifies the asset in RAM.
    void set_source(const char* src, size_t len);
};

struct LuaScriptAssetImportInfo
{
    LuaScriptDomain domain = LUA_SCRIPT_DOMAIN_COMPONENT;
    FS::Path sourcePath; /// path to import the lua script file
    FS::Path savePath;   /// path to save the imported asset
};

class LuaScriptAssetImportJob
{
public:
    LuaScriptAsset asset;          /// subject asset handle
    LuaScriptAssetImportInfo info; /// Blob import configuration

    /// @brief Submit to job system. Address of this job instance must not
    ///        change until the worker thread completes execution.
    void submit();

private:
    static void execute(void*);

    JobHeader mHeader;
};

} // namespace LD