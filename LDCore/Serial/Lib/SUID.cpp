#include <Ludens/DSA/IDCounter.h>
#include <Ludens/Serial/SUID.h>

namespace LD {

HashSet<SUID> SUIDRegistry::sRegistered;
uint32_t SUIDRegistry::sCounter[SERIAL_TYPE_ENUM_COUNT];

SUID SUIDRegistry::get_suid(SerialType type)
{
    if (type == SERIAL_TYPE_NONE)
        return (SUID)0;

    // TODO: if 24-bit space is exhausted for the serial type, this is infinite
    uint32_t i = sCounter[type];
    for (;;)
    {
        if (!sRegistered.contains(i))
        {
            sRegistered.insert(i);
            sCounter[type] = (i + 1) % SUID_IDENTITY_MASK;
            break;
        }

        if (++i == SUID_IDENTITY_MASK)
            i = 0;
    }

    return SUID(type, i);
}

bool SUIDRegistry::try_get_suid(SUID id)
{
    if (!id || sRegistered.contains(id))
        return false;

    sRegistered.insert(id);
    return true;
}

void SUIDRegistry::free_suid(SUID id)
{
    sRegistered.erase(id);
}

} // namespace LD