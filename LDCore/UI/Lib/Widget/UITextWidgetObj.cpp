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

void UITextStorage::set_value(const std::string& newValue)
{
    value = newValue;

    spans.resize(1);
    spans[0].text = {};
    spans[0].text.fgColor = 0xFFFFFFFF;
    spans[0].text.range = Range(0, value.size());
}

void UITextStorage::set_value(const std::string& newValue, const Vector<UITextSpan>& newSpans)
{
    uint32_t base = 0;

    for (size_t i = 0; i < newSpans.size(); i++)
    {
        const Range& range = newSpans[i].text.range;

        if (base != range.offset || range.offset + range.size > newValue.size())
        {
            LD_UNREACHABLE;
            return;
        }

        base = range.offset + range.size;
    }

    spans = newSpans;
    value = newValue;
}

void UITextStorage::set_fg_color(Color fgColor)
{
    for (UITextSpan& span : spans)
        span.text.fgColor = fgColor;
}

std::string UITextStorage::get_substring(int spanIndex)
{
    if (spans.empty())
        return {};

    spanIndex = std::min(spanIndex, (int)spans.size() - 1);
    Range range = spans[spanIndex].text.range;
    return value.substr(0, range.offset + range.size);
}

void UITextWidgetObj::update_span_index(Vec2 localPos)
{
    UIContextObj* ctx = base->ctx();
    TView<UITextSpan> spans(storage->spans.data(), storage->spans.size());
    FontMetrics metrics;

    float lineHeight = 0.0f;
    float ascent = 0.0f;
    size_t spanCount = storage->spans.size();
    Vector<Range> ranges(spanCount);
    Vector<FontAtlas> atlas(spanCount);
    for (size_t i = 0; i < storage->spans.size(); i++)
    {
        UIFont font = ctx->get_font_from_hint(storage->spans[i].text.font);
        ranges[i] = storage->spans[i].text.range;
        atlas[i] = font.font_atlas();
        atlas[i].get_font().get_metrics(metrics, storage->fontSize);
        lineHeight = std::max(lineHeight, (float)metrics.lineHeight);
        ascent = std::max(ascent, (float)metrics.ascent);
    }

    FontGlyphIteration fontIt{};
    fontIt.text = View(storage->value);
    fontIt.lineHeight = lineHeight;
    fontIt.spanAtlas = atlas.data();
    fontIt.spanRange = ranges.data();
    fontIt.spanCount = spanCount;
    fontIt.limitWidth = base->layout.rect.w;
    fontIt.fontSizePx = storage->fontSize;
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

    spanIndex = probe.spanIdx;
}

void UITextWidgetObj::startup(UIWidgetObj* obj, void* storage)
{
    UIContextObj* ctx = obj->ctx();
    auto& self = obj->as.text;
    new (&self) UITextWidgetObj();

    self.base = obj;
    self.storage = (UITextStorage*)storage;

    if (!self.storage)
    {
        obj->flags |= UI_WIDGET_FLAG_LOCAL_STORAGE_BIT;
        self.storage = &self.local;
        *self.storage = {};
    }
}

void UITextWidgetObj::cleanup(UIWidgetObj* base)
{
    UITextWidgetObj& self = base->as.text;

    (&self)->~UITextWidgetObj();
}

bool UITextWidgetObj::on_event(UIWidgetObj* obj, const UIEvent& event)
{
    UITextWidgetObj& self = obj->as.text;
    UITextStorage* storage = self.storage;

    switch (event.type)
    {
    case UI_EVENT_MOUSE_LEAVE:
        self.spanIndex = -1;
        break;
    case UI_EVENT_MOUSE_ENTER:
    case UI_EVENT_MOUSE_DOWN:
    case UI_EVENT_MOUSE_POSITION:
        self.update_span_index(event.mouse.position);
        if (0 <= self.spanIndex && self.spanIndex < storage->spans.size())
        {
            UITextSpan& span = storage->spans[self.spanIndex];
            if (span.onEvent && span.onEvent(UIWidget(obj), event, span, self.spanIndex, span.user))
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
    const UITheme& theme = obj->theme;
    UITextWidgetObj& self = obj->as.text;
    const Rect& rect = obj->layout.rect;
    const UITextStorage* storage = self.storage;
    float wrapWidth = rect.w;

    if (storage->value.empty() && rect.h == 0) // likely a layout bug in UI text wrapping
        LD_DEBUG_BREAK;

    if (storage->bgColor.get_alpha() > 0.0f)
        renderer.draw_rect(rect, storage->bgColor);

    if (storage->spans.empty())
    {
        LD_DEBUG_BREAK; // are u sure?
        return;
    }

    Vec2 pos = rect.get_pos();

    for (const UITextSpan& span : storage->spans)
    {
        UIFont font = ctx.get_font_from_hint(span.text.font);
        FontAtlas atlas = font.font_atlas();
        RImage image = font.image();

        View textView(storage->value.data() + span.text.range.offset, span.text.range.size);
        pos = renderer.draw_text(atlas, image, storage->fontSize, pos, textView, span.text.fgColor, wrapWidth);
    }
}

void UITextWidgetObj::wrap_limit(UIWidgetObj* obj, float& outMinW, float& outMaxW)
{
    LD_ASSERT(obj->type == UI_WIDGET_TEXT);

    UITextWidgetObj& self = obj->as.text;
    UITextStorage* storage = self.storage;
    UIContextObj* ctx = obj->ctx();
    TView<UITextSpan> spans(storage->spans.data(), storage->spans.size());

    // TODO: each span may use a different font.
    ctx->fontDefault.font_atlas().measure_wrap_limit(View(storage->value), storage->fontSize, outMinW, outMaxW);
}

float UITextWidgetObj::wrap_size(UIWidgetObj* obj, float limitW)
{
    LD_ASSERT(obj->type == UI_WIDGET_TEXT);

    UITextWidgetObj& self = obj->as.text;
    UITextStorage* storage = self.storage;
    UIContextObj* ctx = obj->ctx();
    TView<UITextSpan> spans(storage->spans.data(), storage->spans.size());
    View textView(storage->value);

    // TODO: each span may use a different font.
    return ctx->fontDefault.font_atlas().measure_wrap_size(View(storage->value), storage->fontSize, limitW);
}

UITextStorage* UITextWidget::get_storage()
{
    return mObj->as.text.storage;
}

void UITextWidget::set_storage(UITextStorage* storage)
{
    mObj->as.text.storage = storage;
}

void UITextWidget::set_text_style(Color color, TextSpanFont font)
{
    UITextStorage* storage = mObj->as.text.storage;

    for (UITextSpan span : storage->spans)
    {
        span.text.fgColor = color;
        span.text.font = font;
    }
}

int UITextWidget::get_span_index()
{
    return mObj->as.text.spanIndex;
}

} // namespace LD