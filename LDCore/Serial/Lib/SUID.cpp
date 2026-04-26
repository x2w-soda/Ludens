#include <Ludens/Memory/Memory.h>
#include <Ludens/DSA/HashSet.h>
#include <Ludens/Serial/SUID.h>

namespace LD {

struct SUIDRegistryObj
{
    HashSet<SUID> registered;
    uint32_t counter[SERIAL_TYPE_ENUM_COUNT];
};

SUIDRegistry SUIDRegistry::create()
{
    auto* obj = heap_new<SUIDRegistryObj>(MEMORY_USAGE_MISC);

    return SUIDRegistry(obj);
}

void SUIDRegistry::destroy(SUIDRegistry reg)
{
    auto* obj = reg.unwrap();

    heap_delete<SUIDRegistryObj>(obj);
}

SUID SUIDRegistry::get_suid(SerialType type)
{
    if (type == SERIAL_TYPE_NONE)
        return (SUID)0;

    // TODO: if 24-bit space is exhausted for the serial type, this is infinite
    uint32_t i = mObj->counter[type];
    SUID candidate{};

    for (;;)
    {
        candidate = SUID(type, i);

        if (!mObj->registered.contains(candidate))
        {
            mObj->registered.insert(candidate);
            mObj->counter[type] = (i + 1) % SUID_IDENTITY_MASK;
            break;
        }

        if (++i == SUID_IDENTITY_MASK)
            i = 0;
    }

    return candidate;
}

bool SUIDRegistry::try_get_suid(SUID id)
{
    if (!id || mObj->registered.contains(id))
        return false;

    mObj->registered.insert(id);
    return true;
}

void SUIDRegistry::free_suid(SUID id)
{
    mObj->registered.erase(id);
}

bool SUIDRegistry::contains(SUID id)
{
    return id && mObj->registered.contains(id);
}

} // namespace LD
