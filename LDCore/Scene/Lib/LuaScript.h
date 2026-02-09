#pragma once

#include <Ludens/DataRegistry/DataRegistry.h>
#include <Ludens/Lua/LuaModule.h>
#include <Ludens/Lua/LuaState.h>
#include <Ludens/Scene/Scene.h>

namespace LD {
namespace LuaScript {

/// @brief Lua scripting context within a Scene.
class Context
{
public:
    /// @brief In-place startup, initializes lua state.
    void startup(Scene scene, DataRegistry registry, AssetManager assetManager);

    /// @brief In-place cleanup, destroys all scripts and lua state.
    void cleanup();

    /// @brief Set the registry of components.
    void set_registry(DataRegistry registry);

    /// @brief Call update on all scripts.
    /// @param delta Delta time in seconds.
    void update(float delta);

    /// @brief Create table associated with a component.
    void create_component_table(CUID compID);

    /// @brief Destroy table associated with a component, all previous references are invalidated.
    void destroy_component_table(CUID compID);

    /// @brief Creates lua script associated with component if slot is not null.
    bool create_lua_script(CUID compID, AssetID scriptAssetID);

    /// @brief Destroy lua script associated with a component
    void destroy_lua_script(CUID compID);

    /// @brief Attach lua script to its data component.
    void attach_lua_script(CUID compID);

    /// @brief Detach lua script from its data component.
    void detach_lua_script(CUID compID);

private:
    LuaState mL{};
    Scene mScene{};
    DataRegistry mRegistry{};
    AssetManager mAssetManager{};
};

/// @brief Get static C string of LuaScript log channel.
const char* get_log_channel_name();

/// @brief Create the 'ludens' lua module that contains the scripting interface.
LuaModule create_ludens_module();

} // namespace LuaScript
} // namespace LD