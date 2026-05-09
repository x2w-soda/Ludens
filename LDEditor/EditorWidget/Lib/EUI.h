#pragma once

#include <LudensEditor/EditorContext/EditorContext.h>
#include <LudensEditor/EditorContext/EditorContextUtil.h>
#include <LudensEditor/EditorContext/EditorSettings.h>

namespace LD {

void push_prop_hbox();
void pop_prop_hbox();
void push_prop_edit_vbox();
void pop_prop_edit_vbox();
bool push_text_edit_f32(UITextEditData* edit, float* f32, String& str, bool normalized = false);
void pop_text_edit_f32();

EditorTheme eui_get_theme();
EditorContext eui_get_context();

inline void eui_ctx_request_asset(SUID compID, AssetID assetID, AssetType assetType, uint32_t assetSlotIndex = 0)
{
    EditorContextUtil::request_component_asset(eui_get_context(), compID, assetID, assetType, assetSlotIndex);
}

} // namespace LD