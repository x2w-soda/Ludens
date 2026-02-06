#include <Ludens/DSA/IDCounter.h>
#include <Ludens/Header/Assert.h>
#include <Ludens/RenderBackend/RUID.h>

namespace LD {

static IDCounter<RUID> sRUIDCounter;

RUID get_ruid()
{
    RUID id = sRUIDCounter.get_id();
    LD_ASSERT(id); // how do you even exhaust u32 space simultaneously?

    return id;
}

} // namespace LD