#include <Ludens/UI/UITheme.h>

namespace LD {

static UIThemeInfo sDefaultTheme = {
    .surfaceColor = 0x2B2C2FFF,
    .onSurfaceColor = 0xDFDFDFFF,
    .primaryColor = 0x4DD8E6FF,
    .backgroundColor = 0x000000FF,
};

UITheme get_default_theme()
{
    return UITheme(&sDefaultTheme);
}

} // namespace LD