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
    Scene scene{};

    void render_frame(const Vec2& windowExtent);

    /// @brief Callback to inform the render system the transforms of RUIDs.
    static bool render_system_transform_callback(RUID ruid, Mat4& wolrdMat4, void* user);

    static void on_window_event(const WindowEvent* event, void* user);
};

void RuntimeContextObj::render_frame(const Vec2& windowExtent)
{
    const Viewport windowViewport = Viewport::from_extent(windowExtent);

    // begin rendering a frame
    RenderSystemFrameInfo frameI{};
    frameI.directionalLight = Vec3(0.0f, 1.0f, 0.0f);
    frameI.screenExtent = windowExtent;
    frameI.sceneExtent = windowExtent;
    frameI.envCubemap = (RUID)0;
    frameI.clearColor = project.get_settings().get_rendering_settings().get_clear_color();
    renderSystem.next_frame(frameI);

#if 0
    // render game scene without editor overlay
    Camera camera = scene.get_camera();
    LD_ASSERT(camera);
    RenderSystemWorldPass worldP{};
    worldP.mat4Callback = &RuntimeContextObj::render_system_transform_callback;
    worldP.user = this;
    worldP.overlay.enabled = false;
    worldP.hasSkybox = envCubemap.get_id() != 0;
    worldP.worldViewport = screenViewport;
    renderSystem.world_pass(worldP);
#endif

    RenderSystemScreenPass::Region region{};
    region.viewport = windowViewport;

    RenderSystemScreenPass screenP{};
    screenP.overlay.renderCallback = [](ScreenRenderComponent renderer, void* user) {
        RuntimeContextObj& self = *(RuntimeContextObj*)user;
        self.scene.render_screen_ui(renderer);
    };
    screenP.overlay.viewport = windowViewport;
    screenP.regionCount = 1;
    screenP.regions = &region;
    screenP.mat4Callback = &RuntimeContextObj::render_system_transform_callback;
    screenP.user = this;
    renderSystem.screen_pass(screenP);

    renderSystem.submit_frame();
}

bool RuntimeContextObj::render_system_transform_callback(RUID ruid, Mat4& worldMat4, void* user)
{
    RuntimeContextObj& self = *(RuntimeContextObj*)user;

    return self.scene.get_ruid_world_mat4(ruid, worldMat4);
}

void RuntimeContextObj::on_window_event(const WindowEvent* event, void* user)
{
    LD_PROFILE_SCOPE;

    RuntimeContextObj& self = *(RuntimeContextObj*)user;
    LD_ASSERT(self.scene);

    self.scene.input_screen_ui(event);
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
    const std::string windowName = startupS.get_window_name();
    const FS::Path rootPath = obj->project.get_root_path();
    const FS::Path defaultScenePath = rootPath / FS::Path(startupS.get_default_scene_path());
    const FS::Path assetSchemaPath = rootPath / obj->project.get_assets_path();

    WindowInfo windowI{};
    windowI.width = startupS.get_window_width();
    windowI.height = startupS.get_window_height();
    windowI.name = windowName.c_str();
    windowI.onEvent = &RuntimeContextObj::on_window_event;
    windowI.user = obj;
    windowI.hintBorderColor = 0;
    windowI.hintTitleBarColor = 0;
    windowI.hintTitleBarTextColor = 0;
    WindowRegistry::create(windowI);

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

    // initialize subsystems
    RenderSystemInfo systemI{};
    systemI.device = obj->renderDevice;
    systemI.fontAtlas = defaultFont.get_font_atlas();
    obj->renderSystem = RenderSystem::create(systemI);
    obj->audioSystem = AudioSystem::create();

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
    const Vec2 windowExtent = reg.get_window_extent(reg.get_root_id());

    mObj->scene.update(windowExtent, delta);

    mObj->render_frame(windowExtent);
}

} // namespace LD