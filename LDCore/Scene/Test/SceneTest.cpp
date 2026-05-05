#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <Extra/doctest/doctest.h>

#include <Ludens/Memory/Memory.h>
#include <LudensUtil/LudensLFS/LudensLFS.h>
#include <LudensUtil/TestUtil/TestUtil.h>

#include "LuaSceneDriver.h"

using namespace LD;

TEST_CASE("SceneTest" * doctest::skip(!LudensLFS::get_directory_path()))
{
    RDeviceInfo deviceI{};
    deviceI.backend = RDEVICE_BACKEND_VULKAN;
    RDevice device = RDevice::create(deviceI);

    Font font = Font::create_from_path(sLudensLFS.fontPath.string().c_str());
    FontAtlas atlas = FontAtlas::create_bitmap(font, 30.0f);

    Vector<FS::Path> testFiles;
    if (!TestUtil::get_scene_test_files(testFiles))
        return;

    RenderSystemInfo renderSI{};
    renderSI.defaultFontAtlas = atlas;
    renderSI.monoFontAtlas = {};
    renderSI.device = device;
    RenderSystem renderS = RenderSystem::create(renderSI);
    AudioSystem audioS = AudioSystem::create();

    {
        LuaSceneDriver driver(renderS, audioS, atlas);

        for (const FS::Path& file : testFiles)
            CHECK(driver.run(file));
    }

    AudioSystem::destroy(audioS);
    RenderSystem::destroy(renderS);
    FontAtlas::destroy(atlas);
    Font::destroy(font);
    RDevice::destroy(device);

    CHECK_FALSE(get_memory_leaks(nullptr));
}