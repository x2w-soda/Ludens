#pragma once

#include "Core/Math/Include/Vec4.h"
#include "Core/Math/Include/Hex.h"
#include "Core/UI/Include/UI.h"
#include "Core/RenderService/Include/RenderService.h"
#include "Core/RenderService/Lib/RenderContext.h"
#include "Core/RenderService/Lib/RenderUI.h"
#include "Core/UI/Include/Control/Control.h"
#include "Core/UI/Include/Container/Container.h"

namespace LD
{

static void RenderUIWindow(RenderContext* ctx, UIWindow* window);
static void RenderUIWidget(RenderContext* ctx, const Rect2D& rect, UIWidget* widget);
static void RenderUIContainer(RenderContext* ctx, const Rect2D& rect, UIContainerWidget* widget);
static void RenderUIScroll(RenderContext* ctx, const Rect2D& rect, UIScroll* scroll);
static void RenderUILabel(RenderContext* ctx, const Rect2D& rect, UILabel* label);
static void RenderUIPanel(RenderContext* ctx, const Rect2D& rect, UIPanel* panel);
static void RenderUITexture(RenderContext* ctx, const Rect2D& rect, UITexture* texture);
static void RenderUIButton(RenderContext* ctx, const Rect2D& rect, UIButton* button);

void RenderUI(RenderContext* ctx, UIContext* ui)
{
    for (UIWindow* window : ui->GetWindows())
    {
        RenderUIWindow(ctx, window);
    }
}

static void RenderUIWindow(RenderContext* ctx, UIWindow* window)
{
    Rect2D windowRect = window->GetWindowRect();
    Vec4 windowColor = window->GetColor();
    RectBatcher& batcher = ctx->DefaultRectBatcher;
    batcher.AddRectFilled(windowRect, windowColor);

    float border = window->GetBorder();

    Vec4 borderColor = Vec4::Lerp(windowColor, { 1.0f, 1.0f, 1.0f, windowColor.a }, 0.5f);
    batcher.AddRectOutline(windowRect, borderColor, border);

    // render direct child of window
    for (UIWidget* widget : window->GetWidgets())
    {
        Rect2D rect = widget->GetRect();
        rect.x += windowRect.x;
        rect.y += windowRect.y;
        RenderUIWidget(ctx, rect, widget);
    }
}

static void RenderUIWidget(RenderContext* ctx, const Rect2D& rect, UIWidget* widget)
{
    if (widget->GetFlags() & UIWidget::IS_CONTAINER_BIT)
    {
        UIContainerWidget* container = static_cast<UIContainerWidget*>(widget);
        RenderUIContainer(ctx, rect, container);
        return;
    }

    UIType type = widget->GetType();
    switch (type)
    {
    case UIType::Panel:
        RenderUIPanel(ctx, rect, (UIPanel*)widget);
        break;
    case UIType::Label:
        RenderUILabel(ctx, rect, (UILabel*)widget);
        break;
    case UIType::Button:
        RenderUIButton(ctx, rect, (UIButton*)widget);
        break;
    case UIType::Texture:
        RenderUITexture(ctx, rect, (UITexture*)widget);
        break;
    default:
        LD_DEBUG_UNREACHABLE;
    }
}

static void RenderUIContainer(RenderContext* ctx, const Rect2D& rect, UIContainerWidget* container)
{
    UIType type = container->GetType();
    bool commitAndScissor = type == UIType::Scroll;

    if (commitAndScissor)
    {
        ctx->DefaultRectBatcher.Commit();
        ctx->Device.PushScissor(rect);
    }

    switch (type)
    {
    case UIType::Scroll:
        RenderUIScroll(ctx, rect, (UIScroll*)container);
        break;
    default:
        LD_DEBUG_UNREACHABLE;
    }

    for (UIWidget* widget : container->GetWidgets())
    {
        Rect2D widgetRect = container->AdjustedRect(widget->GetRect());
        widgetRect.x += rect.x;
        widgetRect.y += rect.y;
        RenderUIWidget(ctx, widgetRect, widget);
    }

    if (commitAndScissor)
    {
        ctx->DefaultRectBatcher.Commit();
        ctx->Device.PopScissor();
    }
}

static void RenderUIScroll(RenderContext* ctx, const Rect2D& rect, UIScroll* scroll)
{
    RectBatcher& batcher = ctx->DefaultRectBatcher;

    Vec4 color = Hex(0xCC2222FF);
    batcher.AddRectFilled(rect, color);
}

static void RenderUIPanel(RenderContext* ctx, const Rect2D& rect, UIPanel* panel)
{
    RectBatcher& batcher = ctx->DefaultRectBatcher;

    batcher.AddRectFilled(rect, panel->GetColor());
}

static void RenderUILabel(RenderContext* ctx, const Rect2D& rect, UILabel* label)
{
    View<FontGlyphExt> glyphsExts = label->GetTextGlyphs();
    float scale = label->GetGlyphScale();
    UIFont* uiFont = label->GetFont();
    Ref<FontTTF> ttf = uiFont->GetTTF();
    RectBatcher& batcher = ctx->DefaultRectBatcher;

    int ascent;
    ttf->GetVerticalMetrics(&ascent, nullptr, nullptr, nullptr);

    // put cursor at baseline, left most point
    Vec2 cursor = Vec2{ rect.x, rect.y + ascent * scale };

    Vec4 bgColor, fgColor;
    label->GetColors(bgColor, fgColor);

    if (bgColor.a != 0.0f)
        batcher.AddRectFilled(rect, bgColor);

    for (const FontGlyphExt& glyphExt : glyphsExts)
    {
        batcher.AddGlyph(cursor + glyphExt.Offset, glyphExt, scale, fgColor, 1);
    }
}

static void RenderUIButton(RenderContext* ctx, const Rect2D& rect, UIButton* button)
{
    View<FontGlyphExt> glyphsExts = button->GetTextGlyphs();
    float scale = button->GetGlyphScale();
    UIFont* uiFont = button->GetFont();
    Ref<FontTTF> ttf = uiFont->GetTTF();
    RectBatcher& batcher = ctx->DefaultRectBatcher;

    Vec4 bgColor, fgColor;
    button->GetColors(bgColor, fgColor);

    batcher.AddRectFilled(rect, bgColor);

    int ascent;
    ttf->GetVerticalMetrics(&ascent, nullptr, nullptr, nullptr);

    // put cursor at baseline, left most point
    Vec2 cursor = Vec2{ rect.x, rect.y + ascent * scale };

    for (const FontGlyphExt& glyphExt : glyphsExts)
    {
        batcher.AddGlyph(cursor + glyphExt.Offset, glyphExt, scale, fgColor, 1);
    }
}

static void RenderUITexture(RenderContext* ctx, const Rect2D& rect, UITexture* texture)
{
    RectBatcher& batcher = ctx->DefaultRectBatcher;

    Vec4 white(1.0f, 1.0f, 1.0f, 1.0f);
    Vec2 size(rect.w, rect.h);
    batcher.AddTexture(rect, { 0.0f, 0.0f, rect.w, rect.h }, size, white, texture->GetTextureID());
}

} // namespace LD