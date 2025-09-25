#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "UITest.h"
#include <Extra/doctest/doctest.h>
#include <Ludens/Header/Assert.h>
#include <Ludens/System/Memory.h>
#include <Ludens/UI/UIContext.h>
#include <Ludens/UI/UILayout.h>

UITest* UITest::sInstance;

UITest::UITest()
{
    mTheme = UITheme::get_default_info();
    mLFSDirectoryPath.clear();
    mFont = {};
    mFontAtlas = {};

    std::cout << std::filesystem::current_path() << std::endl;

    FS::Path lfsPath;
    if (get_lfs_directory(lfsPath))
    {
        std::cout << "found lfs directory at: " << lfsPath << std::endl;

        FS::Path fontPath = lfsPath / FS::Path("Fonts/Inter_24pt-Regular.ttf");
        LD_ASSERT(FS::exists(fontPath));

        std::string pathString = fontPath.string();
        mFont = Font::create_from_path(pathString.c_str());
        mFontAtlas = FontAtlas::create_bitmap(mFont, 24);
    }
}

UITest* UITest::get()
{
    if (!sInstance)
        sInstance = new UITest();

    return sInstance;
}

bool UITest::get_lfs_directory(FS::Path& lfsDirectory)
{
    const char* candidates[] = {
        "../../../../Ludens/Extra/LudensLFS",
        "../../../../../Ludens/Extra/LudensLFS",
    };

    for (const char* candidate : candidates)
    {
        if (FS::exists(candidate))
        {
            lfsDirectory = candidate;
            return true;
        }
    }

    return false;
}
