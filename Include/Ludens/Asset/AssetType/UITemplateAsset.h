#pragma once

#include <Ludens/Asset/Asset.h>
#include <Ludens/Asset/Template/UITemplate.h>
#include <Ludens/UI/UIContext.h>
#include <cstddef>

namespace LD {

/// @brief UITemplate asset handle.
struct UITemplateAsset : Asset
{
    UIWidget load_ui_subtree(UIWidget parent, UITemplateOnLoadCallback callback, void* user);
};

} // namespace LD