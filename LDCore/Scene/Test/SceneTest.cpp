#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <Extra/doctest/doctest.h>

#include <Ludens/Memory/Memory.h>
#include <Ludens/WindowRegistry/WindowRegistry.h>
#include <LudensBuilder/AssetBuilder/AssetImporter.h>
#include <LudensBuilder/AssetBuilder/AssetState/LuaScriptAssetState.h>
#include <LudensUtil/LudensLFS/LudensLFS.h>
#include <LudensUtil/TestUtil/TestUtil.h>

#include "LuaSceneDriver.h"

using namespace LD;

static AssetRegistry sAssetReg;
static SUIDRegistry sSUIDReg;
static const Vec2 sWindowSize(1280, 720);

// Mock environment for Window, for SceneTests the window could simply be invisible
static void window_startup()
{
    WindowInfo windowI{};
    windowI.width = sWindowSize.x;
    windowI.height = sWindowSize.y;
    windowI.name = "test";
    windowI.invisible = true;
    windowI.onEvent = nullptr;
    REQUIRE(WindowRegistry::create(windowI));
}

static void window_cleanup()
{
    WindowRegistry::destroy();
}

// Mock environment for AssetManager, whlie the Runtime will property setup the asset environment,
// here we just import scripts in the test suite on the fly.
static void asset_manager_startup()
{
    sSUIDReg = SUIDRegistry::create();
    sAssetReg = AssetRegistry::create();

    FS::Path dirPath;
    (void)TestUtil::get_scene_test_dir_path(&dirPath);

    AssetManagerInfo amI{};
    amI.env.registry = sAssetReg;
    amI.env.rootPath = dirPath;
    amI.env.storageDir = dirPath / "Storage";
    amI.watchAssets = false;
    (void)AssetManager::create(amI);

    AssetImporter importer = AssetImporter::create();
    importer.set_resolve_params(sSUIDReg, FS::absolute(amI.env.storageDir));

    Vector<FS::Path> scriptFiles;
    (void)TestUtil::get_scene_test_script_files(scriptFiles);

    for (const FS::Path& scriptFile : scriptFiles)
    {
        auto* importI = (LuaScriptAssetImportInfo*)importer.allocate_import_info(ASSET_TYPE_LUA_SCRIPT);
        importI->domain = LUA_SCRIPT_DOMAIN_COMPONENT;
        importI->srcPath = scriptFile;
        importI->dstPath = scriptFile.stem().string();

        AssetImportResult result = importer.import_asset_synchronous(importI);
        REQUIRE(result.status);
    }

    AssetImporter::destroy(importer);
}

static void asset_manager_cleanup()
{
    AssetManager::destroy();
    AssetRegistry::destroy(sAssetReg);
    sAssetReg = {};
    SUIDRegistry::destroy(sSUIDReg);
    sSUIDReg = {};
}

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

    asset_manager_startup();
    window_startup();

    {
        LuaSceneDriver driver(renderS, audioS, atlas);

        for (const FS::Path& file : testFiles)
            CHECK(driver.run(file));
    }

    window_cleanup();
    asset_manager_cleanup();

    AudioSystem::destroy(audioS);
    RenderSystem::destroy(renderS);
    FontAtlas::destroy(atlas);
    Font::destroy(font);
    RDevice::destroy(device);

    CHECK_FALSE(get_memory_leaks(nullptr));
}