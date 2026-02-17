#include <Ludens/Asset/AssetType/FontAsset.h>
#include <Ludens/Asset/AssetType/TextureCubeAsset.h>
#include <Ludens/AudioSystem/AudioSystem.h>
#include <Ludens/Log/Log.h>
#include <Ludens/Profiler/Profiler.h>
#include <Ludens/Project/Project.h>
#include <Ludens/Project/ProjectSchema.h>
#include <Ludens/RenderBackend/RBackend.h>
#include <Ludens/Scene/Scene.h>
#include <Ludens/Scene/SceneSchema.h>
#include <Ludens/System/FileSystem.h>
#include <Ludens/WindowRegistry/WindowRegistry.h>
#include <LudensRuntime/RuntimeContext/RuntimeContext.h>

namespace LD {

static Log sLog("RuntimeContext");

/// @brief Runtime context implementation.
struct RuntimeContextObj
{
    RDevice renderDevice;
    RenderSystem renderSystem;
    AudioSystem audioSystem;
    AssetManager AM;
    Project project;
    Scene scene;
    ImageCube envCubemap{};

    void render_frame(const Vec2& screenExtent);

    /// @brief Callback to inform the render system the transforms of RUIDs.
    static bool render_system_transform_callback(RUID ruid, Mat4& wolrdMat4, void* user);
};

void RuntimeContextObj::render_frame(const Vec2& screenExtent)
{
    // begin rendering a frame
    RenderSystemFrameInfo frameI{};
    frameI.directionalLight = Vec3(0.0f, 1.0f, 0.0f);
    frameI.mainCamera = scene.get_camera();
    frameI.screenExtent = screenExtent;
    frameI.sceneExtent = screenExtent;
    frameI.envCubemap = envCubemap.get_id();
    frameI.clearColor = project.get_settings().get_rendering_settings().get_clear_color();
    renderSystem.next_frame(frameI);

    // render game scene without editor overlay
    RenderSystemScenePass sceneP{};
    sceneP.mat4Callback = &RuntimeContextObj::render_system_transform_callback;
    sceneP.user = this;
    sceneP.overlay.enabled = false;
    sceneP.hasSkybox = envCubemap.get_id() != 0;
    renderSystem.scene_pass(sceneP);

    renderSystem.submit_frame();
}

bool RuntimeContextObj::render_system_transform_callback(RUID ruid, Mat4& worldMat4, void* user)
{
    RuntimeContextObj& self = *(RuntimeContextObj*)user;

    return self.scene.get_ruid_world_mat4(ruid, worldMat4);
}

//
// PUBLIC API
//

RuntimeContext RuntimeContext::create(const RuntimeContextInfo& info)
{
    LD_PROFILE_SCOPE;

    auto* obj = heap_new<RuntimeContextObj>(MEMORY_USAGE_MISC);
    obj->project = info.project;

    ProjectStartupSettings startupS = obj->project.get_settings().get_startup_settings();
    const FS::Path rootPath = obj->project.get_root_path();
    const FS::Path defaultScenePath = rootPath / FS::Path(startupS.get_default_scene_path());
    const FS::Path assetSchemaPath = rootPath / obj->project.get_assets_path();

    // load assets
    AssetManagerInfo amI{};
    amI.rootPath = rootPath;
    amI.watchAssets = false;
    amI.assetSchemaPath = assetSchemaPath;
    obj->AM = AssetManager::create(amI);
    obj->AM.begin_load_batch();
    obj->AM.load_all_assets();

    // some work on the main thread while worker threads are loading assets
    {
        RDeviceInfo deviceI{};
        deviceI.backend = RDEVICE_BACKEND_VULKAN;
        deviceI.vsync = false; // TODO expose
        obj->renderDevice = RDevice::create(deviceI);
    }

    // this blocks until all worker threads finish loading
    obj->AM.end_load_batch();

    // TODO: generalize
    FontAsset defaultFont = (FontAsset)obj->AM.get_asset("default_font", ASSET_TYPE_FONT);
    LD_ASSERT(defaultFont);
    TextureCubeAsset textureCubeAsset = (TextureCubeAsset)obj->AM.get_asset("default_cubemap", ASSET_TYPE_TEXTURE_CUBE);
    LD_ASSERT(textureCubeAsset);

    // initialize subsystems
    RenderSystemInfo systemI{};
    systemI.device = obj->renderDevice;
    systemI.fontAtlas = defaultFont.get_font_atlas();
    obj->renderSystem = RenderSystem::create(systemI);
    obj->audioSystem = AudioSystem::create();

    obj->envCubemap = obj->renderSystem.create_image_cube(textureCubeAsset.get_bitmap());

    SceneInfo sceneI{};
    sceneI.assetManager = obj->AM;
    sceneI.audioSystem = obj->audioSystem;
    sceneI.renderSystem = obj->renderSystem;
    sceneI.fontAtlas = defaultFont.get_font_atlas();
    sceneI.fontAtlasImage = obj->renderSystem.get_font_atlas_image();
    sceneI.uiTheme = UITheme::get_default_theme();
    obj->scene = Scene::create(sceneI);

    obj->scene.load([&](Scene scene) -> bool {
        // load default scene
        std::string err;
        return SceneSchema::load_scene_from_file(scene, defaultScenePath, err);
    });

    // TODO: check scene load success

    obj->scene.startup();

    return RuntimeContext(obj);
}

void RuntimeContext::destroy(RuntimeContext ctx)
{
    LD_PROFILE_SCOPE;

    auto* obj = (RuntimeContextObj*)ctx.unwrap();
    obj->renderDevice.wait_idle();
    obj->scene.cleanup();
    obj->renderSystem.destroy_image_cube(obj->envCubemap);

    Scene::destroy(obj->scene);
    AudioSystem::destroy(obj->audioSystem);
    RenderSystem::destroy(obj->renderSystem);
    RDevice::destroy(obj->renderDevice);
    AssetManager::destroy(obj->AM);

    heap_delete<RuntimeContextObj>(obj);
}

void RuntimeContext::update(float delta)
{
    LD_PROFILE_SCOPE;

    WindowRegistry reg = WindowRegistry::get();
    const Vec2 screenExtent = reg.get_window_extent(reg.get_root_id());

    mObj->scene.update(screenExtent, delta);

    mObj->render_frame(screenExtent);
}

} // namespace LD