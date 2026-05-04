#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <Extra/doctest/doctest.h>

#include <Ludens/Memory/Memory.h>
#include <LudensUtil/LudensLFS.h>

#include "LuaSceneDriver.h"

using namespace LD;

TEST_CASE("SceneTest")
{
    RDeviceInfo deviceI{};
    deviceI.backend = RDEVICE_BACKEND_VULKAN;
    RDevice device = RDevice::create(deviceI);

    Font font = Font::create_from_path(sLudensLFS.fontPath.string().c_str());
    FontAtlas atlas = FontAtlas::create_bitmap(font, 30.0f);

    FS::Path rootPath;
    if (!LudensLFS::get_root_directory_path(&rootPath))
        return;

    RenderSystemInfo renderSI{};
    renderSI.defaultFontAtlas = atlas;
    renderSI.monoFontAtlas = {};
    renderSI.device = device;
    RenderSystem renderS = RenderSystem::create(renderSI);
    AudioSystem audioS = AudioSystem::create();

    {
        LuaSceneDriver driver(renderS, audioS, atlas);

        FS::Path filePath = FS::absolute(rootPath / "LDCore/Scene/Test/Suite/TransformTest.lua");
        CHECK(driver.run(filePath));
    }

    AudioSystem::destroy(audioS);
    RenderSystem::destroy(renderS);
    FontAtlas::destroy(atlas);
    Font::destroy(font);
    RDevice::destroy(device);

    CHECK_FALSE(get_memory_leaks(nullptr));
}