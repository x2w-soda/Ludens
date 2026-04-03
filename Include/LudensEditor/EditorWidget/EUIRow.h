#pragma once

#include <Ludens/UI/UIImmediate.h>

namespace LD {

bool eui_row_label_text_edit(const char* label, UITextEditStorage* edit, std::string& outText);
bool eui_row_label(int rowIndex, const char* label, bool isHighlighted);
int eui_row_btn_btn(const char* btnLeft, const char* btnRight);

void eui_push_row_scroll(UIScrollStorage* scroll);
void eui_pop_row_scroll();

} // namespace LD
