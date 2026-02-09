#include <Ludens/DSA/IDCounter.h>
#include <Ludens/Header/Assert.h>
#include <Ludens/RenderBackend/RUID.h>

namespace LD {

static IDCounter<RUID> sRUIDCounter;

RUID get_ruid()
{
    return sRUIDCounter.get_id();
}

} // namespace LD