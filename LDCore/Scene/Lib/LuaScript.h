#pragma once

#include <Ludens/DataRegistry/DataComponent.h>
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

    /// @brief Creates lua script associated with component if slot is not null.
    bool create_lua_script(ComponentScriptSlot* scriptSlot);

    /// @brief Destroy lua script associated with a component
    void destroy_lua_script(ComponentScriptSlot* scriptSlot);

    /// @brief Attach lua script to its data component.
    void attach_lua_script(ComponentScriptSlot* scriptSlot);

    /// @brief Detach lua script from its data component.
    void detach_lua_script(ComponentScriptSlot* scriptSlot);

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

/// @brief Create lua table for data component
void create_component_table(LuaState L, CUID compID);

/// @brief Destroy lua table associated with component
void destroy_component_table(LuaState L, CUID compID);

} // namespace LuaScript
} // namespace LD