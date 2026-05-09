#include <Ludens/DSA/ViewUtil.h>
#include <Ludens/UI/Widget/UITextWidget.h>

#include "../UIContextObj.h"
#include "../UIWidgetObj.h"
#include "UITextWidgetObj.h"

namespace LD {

struct TextSpanProbe
{
    int spanIdx = -1; // output span index
    Vec2 pickPos;     // probe position in baseline space
};

void UITextData::clear_value()
{
    mValue.clear();
    mSpans.clear();
}

void UITextData::set_value(View newValue, Color* color)
{
    mValue = String(newValue.data, newValue.size);

    mSpans.resize(1);
    mSpans[0].text.fgColor = color ? *color : Color(0xFFFFFFFF);
    mSpans[0].text.range = Range(0, mValue.size());
}

void UITextData::set_value(View newValue, const Vector<UITextSpan>& newSpans)
{
    uint32_t base = 0;

    for (size_t i = 0; i < newSpans.size(); i++)
    {
        const Range& range = newSpans[i].text.range;

        if (base != range.offset || range.offset + range.size > newValue.size)
        {
            LD_UNREACHABLE;
            return;
        }

        base = range.offset + range.size;
    }

    mSpans = newSpans;
    mValue = String(newValue.data, newValue.size);
}

void UITextData::set_fg_color(Color fgColor)
{
    for (UITextSpan& span : mSpans)
        span.text.fgColor = fgColor;
}

void UITextData::set_span_on_event(UISpanOnEvent onEvent, void* user)
{
    for (UITextSpan& span : mSpans)
    {
        span.user = user;
        span.onEvent = onEvent;
    }
}

String UITextData::get_substring(int spanIndex)
{
    if (mSpans.empty())
        return {};

    spanIndex = std::min(spanIndex, (int)mSpans.size() - 1);
    Range range = mSpans[spanIndex].text.range;
    return mValue.substr(0, range.offset + range.size);
}

void UITextWidgetObj::set_text_style(Color color, TextSpanFont font)
{
    UITextData& data = *(UITextData*)base->data;

    for (UITextSpan& span : data.mSpans)
    {
        span.text.fgColor = color;
        span.text.font = font;
    }
}

void UITextWidgetObj::update_span_index(Vec2 localPos)
{
    UIContextObj* ctx = base->ctx();
    UITextData& data = *(UITextData*)base->data;
    TView<UITextSpan> spans(data.mSpans.data(), data.mSpans.size());
    FontMetrics metrics;

    float lineHeight = 0.0f;
    float ascent = 0.0f;
    size_t spanCount = data.mSpans.size();
    Vector<Range> ranges(spanCount);
    Vector<FontAtlas> atlas(spanCount);
    for (size_t i = 0; i < data.mSpans.size(); i++)
    {
        UIFont font = ctx->get_font_from_hint(data.mSpans[i].text.font);
        ranges[i] = data.mSpans[i].text.range;
        atlas[i] = font.font_atlas();
        atlas[i].get_font().get_metrics(metrics, data.fontSize);
        lineHeight = std::max(lineHeight, (float)metrics.lineHeight);
        ascent = std::max(ascent, (float)metrics.ascent);
    }

    FontGlyphIteration fontIt{};
    fontIt.text = view(data.mValue);
    fontIt.lineHeight = lineHeight;
    fontIt.spanAtlas = atlas.data();
    fontIt.spanRange = ranges.data();
    fontIt.spanCount = spanCount;
    fontIt.limitWidth = base->L->rect.w;
    fontIt.fontSizePx = data.fontSize;
    fontIt.glyphCB = [](Rect rect, Vec2 baseline, size_t charIndex, size_t spanIndex, void* user) -> bool {
        TextSpanProbe* probe = (TextSpanProbe*)user;
        if (probe->spanIdx < 0 && rect.contains(probe->pickPos))
        {
            probe->spanIdx = (int)spanIndex;
            return true;
        }
        return false;
    };

    TextSpanProbe probe{};
    probe.spanIdx = -1;
    probe.pickPos = localPos;
    probe.pickPos.y -= ascent;
    font_glyph_iterator(&fontIt, &probe);

    data.mSpanIndex = probe.spanIdx;
}

UILayoutInfo UITextWidgetObj::default_layout()
{
    return UILayoutInfo(UISize::wrap(), UISize::fit());
}

void UITextWidgetObj::startup(UIWidgetObj* obj)
{
    auto& self = obj->U->text;
    new (&self) UITextWidgetObj();
    self.connect(obj);
}

void UITextWidgetObj::cleanup(UIWidgetObj* obj)
{
    UITextWidgetObj& self = obj->U->text;

    (&self)->~UITextWidgetObj();
}

bool UITextWidgetObj::on_event(UIWidgetObj* obj, const UIEvent& event)
{
    UITextWidgetObj& self = obj->U->text;
    UITextData& data = self.get_data();

    switch (event.type)
    {
    case UI_EVENT_MOUSE_LEAVE:
        data.mSpanIndex = -1;
        break;
    case UI_EVENT_MOUSE_ENTER:
    case UI_EVENT_MOUSE_DOWN:
    case UI_EVENT_MOUSE_POSITION:
        self.update_span_index(event.mouse.position);
        if (0 <= data.mSpanIndex && data.mSpanIndex < data.mSpans.size())
        {
            UITextSpan& span = data.mSpans[data.mSpanIndex];
            if (span.onEvent && span.onEvent(UIWidget(obj), event, span, data.mSpanIndex, span.user))
                return true;
        }
        break;
    default:
        break;
    }

    return false;
}

void UITextWidgetObj::on_draw(UIWidgetObj* obj, ScreenRenderComponent renderer)
{
    UIContextObj& ctx = *obj->ctx();
    UITheme theme = obj->theme;
    UITextWidgetObj& self = obj->U->text;
    const UITextData& data = self.get_data();
    Rect rect = self.get_rect();
    float wrapWidth = rect.w;

    if (data.mValue.empty() && rect.h == 0) // likely a layout bug in UI text wrapping
        LD_DEBUG_BREAK;

    if (data.bgColor.get_alpha() > 0.0f)
        renderer.draw_rect(rect, data.bgColor);

    if (data.mValue.empty() || data.mSpans.empty())
        return;

    Vec2 pos = rect.get_pos();

    for (const UITextSpan& span : data.mSpans)
    {
        UIFont font = ctx.get_font_from_hint(span.text.font);
        FontAtlas atlas = font.font_atlas();
        RImage image = font.image();

        View textView((const byte*)data.mValue.data() + span.text.range.offset, span.text.range.size);
        pos = renderer.draw_text(atlas, image, data.fontSize, pos, textView, span.text.fgColor, wrapWidth);
    }
}

void UITextWidgetObj::wrap_limit(UIWidgetObj* obj, float& outMinW, float& outMaxW)
{
    LD_ASSERT(obj->type == UI_WIDGET_TEXT);

    UITextWidgetObj& self = obj->U->text;
    UITextData& data = self.get_data();
    UIContextObj* ctx = obj->ctx();
    TView<UITextSpan> spans(data.mSpans.data(), data.mSpans.size());

    // TODO: each span may use a different font.
    if (!data.mValue.empty() && spans.size > 0)
    {
        UIFont font = ctx->get_font_from_hint(spans.data[0].text.font);
        font.font_atlas().measure_wrap_limit(view(data.mValue), data.fontSize, outMinW, outMaxW);
    }
}

float UITextWidgetObj::wrap_size(UIWidgetObj* obj, float limitW)
{
    LD_ASSERT(obj->type == UI_WIDGET_TEXT);

    UITextWidgetObj& self = obj->U->text;
    UIContextObj* ctx = obj->ctx();
    UITextData& data = self.get_data();
    TView<UITextSpan> spans(data.mSpans.data(), data.mSpans.size());
    UIFont font = ctx->fontDefault;

    // TODO: each span may use a different font.
    if (!data.mValue.empty() && spans.size > 0)
        font = ctx->get_font_from_hint(spans.data[0].text.font);

    return font.font_atlas().measure_wrap_size(view(data.mValue), data.fontSize, limitW);
}

void UITextWidget::set_text_style(Color color, TextSpanFont font)
{
    mObj->U->text.set_text_style(color, font);
}

int UITextWidget::get_span_index()
{
    UITextWidgetObj& self = mObj->U->text;

    return self.get_data().get_span_index();
}

} // namespace LD