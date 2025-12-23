#include <Ludens/Media/Format/ICO.h>
#include <cstddef>

namespace LD {

// must be identical to ICO file format
// https://en.wikipedia.org/wiki/ICO_(file_format)#File_structure

static_assert(sizeof(ICONDIR) == 6);
static_assert(offsetof(ICONDIR, idReserved) == 0);
static_assert(offsetof(ICONDIR, idType) == 2);
static_assert(offsetof(ICONDIR, idCount) == 4);

static_assert(sizeof(ICONDIRENTRY) == 16);
static_assert(offsetof(ICONDIRENTRY, bWidth) == 0);
static_assert(offsetof(ICONDIRENTRY, bHeight) == 1);
static_assert(offsetof(ICONDIRENTRY, bColorCount) == 2);
static_assert(offsetof(ICONDIRENTRY, bReserved) == 3);
static_assert(offsetof(ICONDIRENTRY, wPlanes) == 4);
static_assert(offsetof(ICONDIRENTRY, wBitCount) == 6);
static_assert(offsetof(ICONDIRENTRY, dwBytesInRes) == 8);
static_assert(offsetof(ICONDIRENTRY, dwImageOffset) == 12);

} // namespace LD