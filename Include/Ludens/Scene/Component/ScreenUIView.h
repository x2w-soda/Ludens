#pragma once

#include <Ludens/Scene/ComponentView.h>

namespace LD {

struct ScreenUIComponent;

/// @brief Public interface for ScreenUI components.
class ScreenUIView : public ComponentView
{
public:
    ScreenUIView() = delete;
    ScreenUIView(ComponentView comp);
    ScreenUIView(ScreenUIComponent* comp);

    bool load(AssetID uiTemplateID);

    bool set_ui_template_asset(AssetID uiTemplateID);
    AssetID get_ui_template_asset();

private:
    ScreenUIComponent* mUI = nullptr;
};

} // namespace LD