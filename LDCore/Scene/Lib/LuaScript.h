#pragma once

#include <Ludens/DataRegistry/DataComponent.h>
#include <Ludens/Lua/LuaModule.h>
#include <Ludens/Lua/LuaState.h>

namespace LD {
namespace LuaScript {

/// @brief Create the 'ludens' lua module that contains the scripting interface.
LuaModule create_ludens_module();

/// @brief Create lua table for data component
void create_component_table(LuaState L, DUID compID, ComponentType type, void* comp);

/// @brief Destroy lua table associated with component
void destroy_component_table(LuaState L, DUID compID);

} // namespace LuaScript
} // namespace LD