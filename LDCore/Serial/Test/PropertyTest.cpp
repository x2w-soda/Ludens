#include <Extra/doctest/doctest.h>
#include <Ludens/DSA/Array.h>
#include <Ludens/Serial/Property.h>

using namespace LD;

struct VecU32
{
    Vector<uint32_t> indices;

public:
    static TypeMeta sTypeMeta;
    static bool get_prop(void* obj, uint32_t propIndex, uint32_t arrayIndex, Value64& val);
    static bool set_prop(void* obj, uint32_t propIndex, uint32_t arrayIndex, const Value64& val);
};

bool VecU32::get_prop(void* obj, uint32_t propIndex, uint32_t arrayIndex, Value64& val)
{
    auto& vec = *(VecU32*)obj;

    if (propIndex != 0 || arrayIndex >= vec.indices.size())
        return false;

    val.set_u32(vec.indices[arrayIndex]);

    return true;
}

bool VecU32::set_prop(void* obj, uint32_t propIndex, uint32_t arrayIndex, const Value64& val)
{
    auto& vec = *(VecU32*)obj;

    if (propIndex != 0 || arrayIndex >= vec.indices.size())
        return false;

    vec.indices[arrayIndex] = val.get_u32();

    return true;
}

static PropertyMeta sVecU32Props[] = {
    {"indices", nullptr, VALUE_TYPE_U32, {}, {}, {}},
};

TypeMeta VecU32::sTypeMeta = {
    .name = "VecU32",
    .props = sVecU32Props,
    .propCount = sizeof(sVecU32Props) / sizeof(*sVecU32Props),
    .getLocal = &VecU32::get_prop,
    .setLocal = &VecU32::set_prop,
};

TEST_CASE("Vector Type")
{
    Value64 val;

    VecU32 v;
    v.indices.resize(8);
    for (size_t i = 0; i < v.indices.size(); i++)
    {
        v.indices[i] = (uint32_t)i;
        CHECK(VecU32::sTypeMeta.getLocal(&v, 0, i, val));
        CHECK(val.get_u32() == i);
    }

    for (size_t i = 0; i < v.indices.size(); i++)
    {
        v.indices[i] = (uint32_t)i;
        val.set_u32(i * 2);
        CHECK(VecU32::sTypeMeta.setLocal(&v, 0, i, val));
        CHECK(v.indices[i] == i * 2);
    }
}