#include <Ludens/DSA/Vector.h>
#include <Ludens/Header/Assert.h>
#include <Ludens/Header/Hash.h>
#include <Ludens/Header/Math/Vec3.h>
#include <Ludens/Log/Log.h>
#include <Ludens/Lua/LuaConfig.h>
#include <Ludens/Lua/LuaState.h>
#include <Ludens/Profiler/Profiler.h>
#include <Ludens/System/Memory.h>
#include <cstdint>
#include <cstring>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace LD {

static Log sLog("LuaConfig");

// clang-format off
static struct
{
    LuaConfigType configType;
    const char* configTypeName;
    LuaType type;
} sTable[]{
    {LUA_CONFIG_TYPE_BOOL,   "boolean", LUA_TYPE_BOOL},
    {LUA_CONFIG_TYPE_I32,    "i32",     LUA_TYPE_NUMBER},
    {LUA_CONFIG_TYPE_F64,    "f64",     LUA_TYPE_NUMBER},
    {LUA_CONFIG_TYPE_STRING, "string",  LUA_TYPE_STRING},
    {LUA_CONFIG_TYPE_VEC2,   "Vec2",    LUA_TYPE_TABLE},
    {LUA_CONFIG_TYPE_VEC3,   "Vec3",    LUA_TYPE_TABLE},
    {LUA_CONFIG_TYPE_VEC4,   "Vec4",    LUA_TYPE_TABLE},
};
// clang-format on

struct LuaConfigEntry
{
    union Value
    {
        bool b;             /// LUA_CONFIG_TYPE_BOOL
        int32_t i32;        /// LUA_CONFIG_TYPE_I32
        double f64;         /// LUA_CONFIG_TYPE_F64
        const char* string; /// LUA_CONFIG_TYPE_STRING
        Vec2 v2;            /// LUA_CONFIG_TYPE_VEC2
        Vec3 v3;            /// LUA_CONFIG_TYPE_VEC3
        Vec4 v4;            /// LUA_CONFIG_TYPE_VEC4
    };

    LuaConfigType type;
    SVector<Value, 1, MEMORY_USAGE_LUA> values; /// does not heap-allocate if arrayCount equals 1
};

struct LuaConfigObj
{
    std::string name;                                     /// configuration name
    LuaState L;                                           /// temporary state to evaluate configuration file
    std::vector<LuaConfigValue> values;                   /// configuration value schema
    std::unordered_map<uint32_t, LuaConfigEntry> entries; /// configuration values extracted from lua code
    std::unordered_set<uint32_t> valueNames;              /// configuration value hashed names
    bool isLoaded;                                        /// whether lua configuration code is loaded before or not

    int get_entry(Hash32 name, LuaConfigEntry& entry);

    void load_entry(const LuaConfigValue& value, uint32_t key);
};

int LuaConfigObj::get_entry(Hash32 name, LuaConfigEntry& entry)
{
    if (!isLoaded)
        return (int)LUA_CONFIG_ERROR_NOT_LOADED;

    if (!valueNames.contains(name))
        return (int)LUA_CONFIG_ERROR_NOT_REGISTERED;

    const auto& ite = entries.find(name);
    if (ite == entries.end())
        return LUA_CONFIG_ERROR_NOT_FOUND;

    entry = ite->second;
    return 1; // TODO: entry.arrayCount
}

// loads a single configuration entry
void LuaConfigObj::load_entry(const LuaConfigValue& value, uint32_t key)
{
    LuaConfigEntry entry;
    entry.type = value.type;
    entry.values.resize(value.arrayCount);

    for (int i = 1; i <= value.arrayCount; i++)
    {
        LD_ASSERT(L.size() == 2); // config table at index 1, current value at index 2

        if (value.arrayCount > 1)
        {
            LD_ASSERT(L.get_type(-1) == LUA_TYPE_TABLE); // array table
            L.push_integer(i);
            L.get_table(-2);
        }

        LuaConfigEntry::Value v{};

        switch (value.type)
        {
        case LUA_CONFIG_TYPE_BOOL:
            v.b = L.to_bool(-1);
            L.pop(1);
            break;
        case LUA_CONFIG_TYPE_I32:
            v.i32 = L.to_integer(-1);
            L.pop(1);
            break;
        case LUA_CONFIG_TYPE_F64:
            v.f64 = L.to_number(-1);
            L.pop(1);
            break;
        case LUA_CONFIG_TYPE_STRING:
            v.string = L.to_string(-1);
            L.pop(1);
            break;
        case LUA_CONFIG_TYPE_VEC2:
            L.get_table_indices(-1, 1, 2);
            v.v2.x = (float)L.to_number(-2);
            v.v2.y = (float)L.to_number(-1);
            L.pop(3);
            break;
        case LUA_CONFIG_TYPE_VEC3:
            L.get_table_indices(-1, 1, 3);
            v.v3.x = (float)L.to_number(-3);
            v.v3.y = (float)L.to_number(-2);
            v.v3.z = (float)L.to_number(-1);
            L.pop(4);
            break;
        case LUA_CONFIG_TYPE_VEC4:
            L.get_table_indices(-1, 1, 4);
            v.v4.x = (float)L.to_number(-4);
            v.v4.y = (float)L.to_number(-3);
            v.v4.z = (float)L.to_number(-2);
            v.v4.w = (float)L.to_number(-1);
            L.pop(5);
            break;
        default:
            LD_UNREACHABLE;
        }

        entry.values[i - 1] = v;
    }

    if (value.arrayCount > 1)
        L.pop(1); // pop the current value (array table)

    LD_ASSERT(L.size() == 1); // config table only

    entries[key] = entry;
}

LuaConfig LuaConfig::create(const LuaConfigInfo& info)
{
    LuaConfigObj* obj = heap_new<LuaConfigObj>(MEMORY_USAGE_LUA);

    LuaStateInfo stateI{};
    stateI.openLibs = true;
    obj->L = LuaState::create(stateI);
    obj->name = info.name;
    obj->isLoaded = false;
    obj->values.resize((size_t)info.valueCount);
    std::copy(info.values, info.values + info.valueCount, obj->values.data());

    return {obj};
}

void LuaConfig::destroy(LuaConfig config)
{
    LuaConfigObj* obj = config;
    LuaState::destroy(obj->L);

    heap_delete<LuaConfigObj>(obj);
}

bool LuaConfig::load(const char* src)
{
    LD_PROFILE_SCOPE;

    mObj->L.clear();
    mObj->isLoaded = true;
    LuaState L = mObj->L;

    if (!L.do_string(src))
    {
        sLog.warn("{}: failed to evaluate lua code", mObj->name);
        return false;
    }

    if ((L.size() != 1) || (L.get_type(-1) != LUA_TYPE_TABLE))
    {
        sLog.warn("{}: lua code should return a table as the configuration", mObj->name);
        return false;
    }

    // extract configuration values from lua state
    mObj->entries.clear();

    for (const LuaConfigValue& value : mObj->values)
    {
        L.get_field(-1, value.name.c_str());
        LuaType entryType = L.get_type(-1);
        uint32_t key = (uint32_t)Hash32(value.name);
        mObj->valueNames.insert(key);

        if (entryType == LUA_TYPE_NIL)
        {
            sLog.warn("{}: configuration value for {} not found", mObj->name, value.name);
            L.pop(1);
            continue;
        }

        if (value.arrayCount <= 0)
        {
            sLog.warn("{}: invalid array count {} for {}", mObj->name, value.arrayCount, value.name);
            L.pop(1);
            continue;
        }

        LuaType expectedType = sTable[(int)value.type].type;
        if (value.arrayCount > 1)
            expectedType = LUA_TYPE_TABLE; // array table

        if (entryType != expectedType)
        {
            sLog.warn("{}: configuration type mismatch for {}", mObj->name, value.name);
            L.pop(1);
            continue;
        }

        LD_ASSERT(L.size() == 2); // config table at index 1, current value at index 2
        mObj->load_entry(value, key);
    }

    return true;
}

int LuaConfig::get_bool(const char* name, bool* b)
{
    LuaConfigEntry entry;
    int ret = mObj->get_entry(name, entry);

    if (ret <= 0)
        return ret;

    int count = (int)entry.values.size();
    for (int i = 0; i < count; i++)
        b[i] = entry.values[i].b;

    return count;
}

int LuaConfig::get_i32(const char* name, int32_t* i32)
{
    LuaConfigEntry entry;
    int ret = mObj->get_entry(name, entry);

    if (ret <= 0)
        return ret;

    int count = (int)entry.values.size();
    for (int i = 0; i < count; i++)
        i32[i] = entry.values[i].i32;

    return count;
}

int LuaConfig::get_f64(const char* name, double* f64)
{
    LuaConfigEntry entry;
    int ret = mObj->get_entry(name, entry);

    if (ret <= 0)
        return ret;

    int count = (int)entry.values.size();
    for (int i = 0; i < count; i++)
        f64[i] = entry.values[i].f64;

    return count;
}

int LuaConfig::get_string(const char* name, const char** string)
{
    LuaConfigEntry entry;
    int ret = mObj->get_entry(name, entry);

    if (ret <= 0)
        return ret;

    int count = (int)entry.values.size();
    for (int i = 0; i < count; i++)
        string[i] = entry.values[i].string;

    return count;
}

int LuaConfig::get_vec2(const char* name, Vec2* v)
{
    LuaConfigEntry entry;
    int ret = mObj->get_entry(name, entry);

    if (ret <= 0)
        return ret;

    int count = (int)entry.values.size();
    for (int i = 0; i < count; i++)
        v[i] = entry.values[i].v2;

    return count;
}

int LuaConfig::get_vec3(const char* name, Vec3* v)
{
    LuaConfigEntry entry;
    int ret = mObj->get_entry(name, entry);

    if (ret <= 0)
        return ret;

    int count = (int)entry.values.size();
    for (int i = 0; i < count; i++)
        v[i] = entry.values[i].v3;

    return count;
}

int LuaConfig::get_vec4(const char* name, Vec4* v)
{
    LuaConfigEntry entry;
    int ret = mObj->get_entry(name, entry);

    if (ret <= 0)
        return ret;

    int count = (int)entry.values.size();
    for (int i = 0; i < count; i++)
        v[i] = entry.values[i].v4;

    return count;
}

} // namespace LD
