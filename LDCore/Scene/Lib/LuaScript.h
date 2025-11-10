#pragma once

#include <Ludens/DataRegistry/DataComponent.h>
#include <Ludens/DataRegistry/DataRegistry.h>
#include <Ludens/Lua/LuaModule.h>
#include <Ludens/Lua/LuaState.h>
#include <Ludens/Scene/Scene.h>

namespace LD {
namespace LuaScript {

/// @brief Get static C string of LuaScript log channel.
const char* get_log_channel_name();

/// @brief Create the 'ludens' lua module that contains the scripting interface.
LuaModule create_ludens_module();

/// @brief Create lua table for data component
void create_component_table(Scene scene, DataRegistry reg, LuaState L, CUID compID, ComponentType type, void* comp);

/// @brief Destroy lua table associated with component
void destroy_component_table(Scene scene, DataRegistry reg, LuaState L, CUID compID);

} // namespace LuaScript
} // namespace LD