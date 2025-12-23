#include <Ludens/Media/Win32Struct.h>
#include <cstddef>

namespace LD {

static_assert(sizeof(GRPICONDIR) == 6);
static_assert(offsetof(GRPICONDIR, idReserved) == 0);
static_assert(offsetof(GRPICONDIR, idType) == 2);
static_assert(offsetof(GRPICONDIR, idCount) == 4);

static_assert(sizeof(GRPICONDIRENTRY) == 14);
static_assert(offsetof(GRPICONDIRENTRY, bWidth) == 0);
static_assert(offsetof(GRPICONDIRENTRY, bHeight) == 1);
static_assert(offsetof(GRPICONDIRENTRY, bColorCount) == 2);
static_assert(offsetof(GRPICONDIRENTRY, bReserved) == 3);
static_assert(offsetof(GRPICONDIRENTRY, wPlanes) == 4);
static_assert(offsetof(GRPICONDIRENTRY, wBitCount) == 6);
static_assert(offsetof(GRPICONDIRENTRY, dwBytesInRes) == 8);
static_assert(offsetof(GRPICONDIRENTRY, nId) == 12);

} // namespace LD