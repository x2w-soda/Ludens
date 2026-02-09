#include <Ludens/DSA/IDCounter.h>
#include <Ludens/Serial/SUID.h>

namespace LD {

// NOTE: this should technically be main thread only,
//       but we can make this thread safe later.
static IDRegistry<SUID> sSUIDRegistry;

SUID get_suid()
{
    return sSUIDRegistry.get_id();
}

bool try_get_suid(SUID id)
{
    return sSUIDRegistry.try_get_id(id);
}

void free_suid(SUID id)
{
    return sSUIDRegistry.free(id);
}

} // namespace LD