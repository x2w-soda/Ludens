#include <Ludens/UI/UITheme.h>

namespace LD {

void get_default_theme(UITheme& theme)
{
    theme.surfaceColor = 0x0F1C1DFF;
    theme.onSurfaceColor = 0xDAE6E8FF;
    theme.primaryColor = 0x4DD8E6FF;
    theme.onPrimaryColor = 0x00363AFF;
    theme.backgroundColor = 0x0F1C1DFF;
    theme.onBackgroundColor = 0xDAE6E8FF;
    theme.primaryContainerColor = 0x004F56FF;
    theme.onPrimaryContainerColor = 0xA6F5FFFF;
}

} // namespace LD