#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <Extra/doctest/doctest.h>
#include <Ludens/Header/Assert.h>
#include <Ludens/Memory/Memory.h>
#include <Ludens/UI/UIContext.h>
#include <Ludens/UI/UILayout.h>
#include <LudensUtil/LudensLFS.h>

#include "UITest.h"

UITest* UITest::sInstance;

UITest::UITest()
{
    mTheme = UITheme::get_default_theme();

    if (sLudensLFS.isFound)
    {
        std::string pathString = sLudensLFS.fontPath.string();
        mFont = Font::create_from_path(pathString.c_str());
        mFontAtlas = FontAtlas::create_bitmap(mFont, 24);
        LD_ASSERT(mFont && mFontAtlas);
    }

    std::cout << std::filesystem::current_path() << std::endl;
}

UITest* UITest::get()
{
    if (!sInstance)
        sInstance = new UITest();

    return sInstance;
}
