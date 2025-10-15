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

    static inline bool found_lfs_directory()
    {
        FS::Path lfsPath;
        return get_lfs_directory(lfsPath);
    }

    static inline UIContext create_test_context()
    {
        UITest* instance = get();

        UIContextInfo ctxI;
        ctxI.fontAtlas = instance->mFontAtlas;
        ctxI.fontAtlasImage = {};
        ctxI.theme = UITheme(&instance->mTheme);
        UIContext ctx = UIContext::create(ctxI);

        // lazy default layer
        uint32_t layerHash = 0;
        ctx.add_layer(layerHash);

        return ctx;
    }

private:
    static UITest* sInstance;

    FS::Path mLFSDirectoryPath;
    UIThemeInfo mTheme;
    Font mFont;
    FontAtlas mFontAtlas;

    static UITest* get();

    static bool get_lfs_directory(FS::Path& lfsDirectory);
};