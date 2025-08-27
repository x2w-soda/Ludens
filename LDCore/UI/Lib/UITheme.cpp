#include <Ludens/UI/UITheme.h>

namespace LD {

static const UIThemeInfo sDefaultThemeInfo = {
    .surfaceColor = 0x2B2C2FFF,
    .onSurfaceColor = 0xDFDFDFFF,
    .primaryColor = 0x4DD8E6FF,
    .backgroundColor = 0x000000FF,
};

UIThemeInfo UITheme::get_default_info()
{
    return sDefaultThemeInfo;
}

} // namespace LD