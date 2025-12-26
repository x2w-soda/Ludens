#include <Ludens/Media/Format/BMP.h>

namespace LD {

static_assert(sizeof(BITMAPFILEHEADER) == 14);
static_assert(alignof(BITMAPFILEHEADER) == 2);

static_assert(sizeof(BITMAPINFOHEADER) == 40);
static_assert(alignof(BITMAPINFOHEADER) == 2);

} // namespace LD