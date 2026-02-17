#include <Ludens/UI/UITheme.h>

namespace LD {

static UIThemeInfo sDefaultThemeInfo = {
    .surfaceColor = 0x2B2C2FFF,
    .onSurfaceColor = 0xDFDFDFFF,
    .primaryColor = 0x4DD8E6FF,
    .backgroundColor = 0x000000FF,
    .fieldColor = 0x1B1B1BFF,
    .selectionColor = 0x4D6490FF,
};

UITheme UITheme::get_default_theme()
{
    return UITheme(&sDefaultThemeInfo);
}

} // namespace LD