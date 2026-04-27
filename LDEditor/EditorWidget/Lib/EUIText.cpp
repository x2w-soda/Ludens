#include <Ludens/Header/Math/Vec2.h>
#include <Ludens/Header/MouseValue.h>
#include <Ludens/Text/Text.h>
#include <LudensEditor/EditorWidget/EUIText.h>
#include <LudensEditor/EditorWidget/EditorWidget.h>

#include "EUI.h"

namespace LD {

static void breadcrumb_text_to_spans(const std::string& str, Vector<UITextSpan>& outSpans)
{
    size_t offset = 0;

    const Vector<Range> ranges = text_split_ranges(View(str.data(), str.size()), '/', true);
    outSpans.resize(ranges.size());

    for (size_t i = 0; i < ranges.size(); i++)
    {
        outSpans[i] = {};
        outSpans[i].text.fgColor = 0xFFFFFFFF;
        outSpans[i].text.range = ranges[i];
    }
}

bool EUIText::update(const char* label, float height, Rect* outRect)
{
    bool isPressed = false;
    EditorTheme theme = eui_get_theme();
    MouseValue mouseVal;
    Vec2 mousePos;
    const float pad = theme.get_child_pad();

    ui_push_panel(&mPanel);
    UILayoutInfo layoutI(UISize::fit(), UISize::fixed(height));
    layoutI.childPadding = UIPadding::left_right(pad / 2.0f, pad / 2.0f);
    ui_top_layout(layoutI);
    mPanel.color = 0;
    mPanel.radius = radius;
    if (ui_top_is_hovered())
        mPanel.color = theme.get_ui_theme().get_surface_color_lifted();

    if (outRect)
        ui_top_get_rect(*outRect);

    {
        layoutI.sizeX = UISize::wrap();
        layoutI.sizeY = UISize::fixed(height);
        ui_push_text(&mText, label);
        ui_top_layout(layoutI);

        if (ui_top_mouse_down(mouseVal, mousePos))
            isPressed = true;

        ui_pop();
    }
    ui_pop();

    return isPressed;
}

void EUITextBreadcrumb::build(const char* cstr)
{
    Vector<UITextSpan> spans;
    breadcrumb_text_to_spans(cstr, spans);
    mText.set_value(cstr, spans);
}

std::string EUITextBreadcrumb::update(float height, Color hlColor)
{
    EditorTheme theme = eui_get_theme();
    std::string str;

    UILayoutInfo layoutI{};
    layoutI.sizeX = UISize::fit();
    layoutI.sizeY = UISize::fixed(height);
    layoutI.childPadding = UIPadding::left_right(4.0f, 4.0f);
    ui_push_panel(&mPanel);
    ui_top_layout(layoutI);
    {
        layoutI.sizeX = UISize::wrap();
        layoutI.sizeY = UISize::fixed(height);

        ui_push_text(&mText);
        ui_top_layout(layoutI);

        Vector<UITextSpan>& spans = mText.get_spans();
        const std::string& value = mText.get_value();

        for (size_t i = 0; i < spans.size(); i++)
        {
            TextSpan& span = spans[i].text;
            Color spanTextColor = theme.get_ui_theme().get_on_surface_color();

            if (span.range.size == 1 && value[span.range.offset] == '/')
                continue;

            if (ui_text_span_hovered(i))
            {
                spanTextColor = hlColor;
                eui_set_window_cursor(CURSOR_TYPE_HAND);
            }

            span.fgColor = spanTextColor;

            if (ui_text_span_pressed(i))
                str = mText.get_substring(i);
        }

        ui_pop();
    }
    ui_pop();

    return str;
}

} // namespace LD
