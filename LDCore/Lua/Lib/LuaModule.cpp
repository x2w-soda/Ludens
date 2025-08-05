#include <Ludens/Header/Assert.h>
#include <Ludens/Lua/LuaModule.h>
#include <Ludens/Profiler/Profiler.h>
#include <Ludens/System/Memory.h>
#include <string>
#include <vector>

namespace LD {

struct LuaNamespaceObj
{
    // TODO: do not cache pointer types
    std::vector<LuaModuleValue> values;
};

struct LuaModuleObj
{
    std::string name;
    std::vector<LuaNamespaceObj*> spaces;
};

LuaModule LuaModule::create(const LuaModuleInfo& moduleI)
{
    LuaModuleObj* obj = heap_new<LuaModuleObj>(MEMORY_USAGE_LUA);

    obj->name = moduleI.name;
    obj->spaces.resize(moduleI.spaceCount);
    for (uint32_t i = 0; i < moduleI.spaceCount; i++)
    {
        LuaNamespaceObj* spaceObj = obj->spaces[i] = heap_new<LuaNamespaceObj>(MEMORY_USAGE_LUA);
        const LuaModuleNamespace* space = moduleI.spaces + i;

        spaceObj->values.resize(space->valueCount);
        std::copy(space->values, space->values + space->valueCount, spaceObj->values.data());
    }

    return {obj};
}

void LuaModule::destroy(LuaModule mod)
{
    LuaModuleObj* obj = mod;

    for (LuaNamespaceObj* spaceObj : obj->spaces)
        heap_delete<LuaNamespaceObj>(spaceObj);

    heap_delete<LuaModuleObj>(obj);
}

void LuaModule::load(LuaState& L)
{
    LD_PROFILE_SCOPE;

    int s = L.size();

    L.get_global("package");
    L.get_field(-1, "loaded");
    L.push_table(); // module

    for (const LuaNamespaceObj* spaceObj : mObj->spaces)
    {
        for (const LuaModuleValue& value : spaceObj->values)
        {
            switch (value.type)
            {
            case LUA_TYPE_STRING:
                L.push_string(value.string);
                break;
            case LUA_TYPE_NUMBER:
                L.push_number(value.number);
                break;
            case LUA_TYPE_FN:
                L.push_fn(value.fn);
                break;
            }

            L.set_field(-2, value.name);
        }
    }

    // package.loaded[modname] = module
    const char* modname = mObj->name.c_str();
    L.set_field(-2, modname);
    L.pop(2);
    LD_ASSERT(s == L.size());
}

} // namespace LD