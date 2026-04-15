#pragma once

#include <Ludens/UI/UIImmediate.h>

namespace LD {

class UITextEditData;

bool eui_row_label_text_edit(const char* label, UITextEditData* edit, std::string& outText);
bool eui_row_label(int rowIndex, const char* label, bool isHighlighted);
int eui_row_btn_btn(UIButtonData* btnLeft, UIButtonData* btnRight);

void eui_push_row_scroll(UIScrollData* scroll);
void eui_pop_row_scroll();

} // namespace LD
