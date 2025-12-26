#include <Ludens/Media/Format/PNG.h>
#include <cstring>

namespace LD {

static constexpr char sPNGMagic[] = "\x89\x50\x4E\x47\x0D\x0A\x1A\x0A";

bool PNGData::test_magic(const void* fileData, size_t fileSize)
{
    if (!fileData || fileSize < 8)
        return false;

    return !strncmp((const char*)fileData, sPNGMagic, 8);
}

} // namespace LD