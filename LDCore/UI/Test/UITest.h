#pragma once

#include <Ludens/System/FileSystem.h>
#include <Ludens/UI/UIContext.h>
#include <Ludens/UI/UILayout.h>
#include <iostream>

using namespace LD;

inline UILayoutInfo make_fit_layout()
{
    UILayoutInfo layoutI{};
    layoutI.childAxis = UI_AXIS_X;
    layoutI.childGap = 0.0f;
    layoutI.sizeX = UISize::fit();
    layoutI.sizeY = UISize::fit();
    return layoutI;
}

inline UILayoutInfo make_fixed_size_layout(float sizeX, float sizeY)
{
    UILayoutInfo layoutI{};
    layoutI.childAxis = UI_AXIS_X;
    layoutI.childGap = 0.0f;
    layoutI.sizeX = UISize::fixed(sizeX);
    layoutI.sizeY = UISize::fixed(sizeY);
    return layoutI;
}

class UITest
{
public:
    UITest();

    /// @brief Creates a context with a single layer with a single workspace.
    static inline UIContext create_test_context(const Vec2& workspaceExtent, UIWorkspace& outWorkspace)
    {
        UITest* instance = get();

        UIContextInfo ctxI;
        ctxI.fontAtlas = instance->mFontAtlas;
        ctxI.fontAtlasImage = {};
        ctxI.theme = instance->mTheme;
        UIContext ctx = UIContext::create(ctxI);

        UILayer layer = ctx.create_layer("test");
        outWorkspace = layer.create_workspace(Rect(0.0f, 0.0f, workspaceExtent.x, workspaceExtent.y));

        return ctx;
    }

private:
    static UITest* sInstance;
    static UITest* get();

private:
    UITheme mTheme;
    Font mFont{};
    FontAtlas mFontAtlas{};
};