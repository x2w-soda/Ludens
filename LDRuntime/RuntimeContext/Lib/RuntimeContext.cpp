#include <Ludens/Asset/AssetRegistry.h>
#include <Ludens/Asset/AssetSchema.h>
#include <Ludens/Asset/AssetType/FontAsset.h>
#include <Ludens/Asset/AssetType/TextureCubeAsset.h>
#include <Ludens/AudioSystem/AudioSystem.h>
#include <Ludens/Log/Log.h>
#include <Ludens/Profiler/Profiler.h>
#include <Ludens/Project/Project.h>
#include <Ludens/Project/ProjectContext.h>
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
    Scene scene = {};
    ProjectContext projectCtx = {};
    UIFontRegistry fontRegistry;
    UIFont fontDefault;

    bool startup(const RuntimeContextInfo& info, std::string err);
    void cleanup();
    void render_frame(const Vec2& windowExtent);

    /// @brief Callback to inform the render system the transforms of RUIDs.
    static bool render_system_transform_callback(RUID ruid, Mat4& wolrdMat4, void* user);

    static void on_window_event(const WindowEvent* event, void* user);
};

bool RuntimeContextObj::startup(const RuntimeContextInfo& info, std::string err)
{
    if (!projectCtx.load_project_schema(info.projectSchemaPath, err))
        return false;

    Project project = projectCtx.project();
    ProjectSettings projectS = project.settings();
    ProjectStartupSettings startupS = projectS.startup_settings();
    ProjectScreenLayerSettings screenLayerS = projectS.screen_layer_settings();
    ProjectSceneEntry startupScene;
    if (!project.get_scene(startupS.get_default_scene_id(), startupScene))
    {
        err = "failed to locate default scene in project";
        return false;
    }

    const std::string windowName = startupS.get_window_name();
    const FS::Path rootPath = project.get_root_path();
    const FS::Path defaultScenePath = rootPath / FS::Path(startupScene.path);
    const FS::Path assetSchemaPath = projectCtx.asset_schema_abs_path();

    if (!projectCtx.load_asset_schema(assetSchemaPath, err))
        return false;

    WindowInfo windowI{};
    windowI.width = startupS.get_window_width();
    windowI.height = startupS.get_window_height();
    windowI.name = windowName.c_str();
    windowI.onEvent = &RuntimeContextObj::on_window_event;
    windowI.user = this;
    windowI.hintBorderColor = 0;
    windowI.hintTitleBarColor = 0;
    windowI.hintTitleBarTextColor = 0;
    WindowRegistry::create(windowI);

    sLog.info("begin loading assets [{}]", assetSchemaPath.string());

    // load assets
    AssetManagerInfo amI{};
    amI.watchAssets = false;
    amI.registry = projectCtx.asset_registry();
    amI.rootPath = rootPath;
    AssetManager AM = AssetManager::create(amI);
    AM.begin_load_batch();
    AM.load_all_assets();

    // some work on the main thread while worker threads are loading assets
    {
        RDeviceInfo deviceI{};
        deviceI.backend = RDEVICE_BACKEND_VULKAN;
        deviceI.vsync = false; // TODO expose
        renderDevice = RDevice::create(deviceI);
    }

    // this blocks until all worker threads finish loading
    Vector<std::string> loadErrors;
    if (!AM.end_load_batch(loadErrors))
    {
        sLog.error("AssetManager failed to load assets with {} errors", loadErrors.size());
        for (const std::string& err : loadErrors)
            sLog.error("{}", err);

        return false;
    }

    sLog.info("finish loading assets");

    // TODO: generalize
    FontAsset defaultFont = (FontAsset)AM.get_asset("default_font", ASSET_TYPE_FONT);
    if (!defaultFont)
    {
        LD_DEBUG_BREAK;
        return false;
    }

    // initialize subsystems
    RenderSystemInfo systemI{};
    systemI.device = renderDevice;
    systemI.defaultFontAtlas = defaultFont.get_font_atlas();
    systemI.monoFontAtlas = {};
    renderSystem = RenderSystem::create(systemI);
    audioSystem = AudioSystem::create();

    fontRegistry = UIFontRegistry::create();
    fontDefault = fontRegistry.add_font(defaultFont.get_font_atlas(), renderSystem.get_font_atlas_image());

    Vector<ProjectScreenLayer> layers = screenLayerS.get_layers();

    SceneInfo sceneI{};
    sceneI.audioSystem = audioSystem;
    sceneI.renderSystem = renderSystem;
    sceneI.uiFont = fontDefault;
    sceneI.uiTheme = UITheme::get_default_theme();
    scene = Scene::create(sceneI);

    // Need at least one ScreenLayer before loading any Sprite2DComponents
    projectCtx.configure_project_screen_layers();

    scene.load([&](SceneObj* sceneObj) -> bool {
        // load default scene
        std::string err;
        return SceneSchema::load_scene_from_file(Scene(sceneObj), projectCtx.suid_registry(), defaultScenePath, err);
    });

    // TODO: check scene load success
    sLog.info("load complete");

    if (!scene.startup())
    {
        sLog.error("default scene failed to startup");
        return false;
    }

    sLog.info("startup complete");

    return true;
}

void RuntimeContextObj::cleanup()
{
    renderDevice.wait_idle();
    scene.cleanup();

    Scene::destroy();
    UIFontRegistry::destroy(fontRegistry);
    AudioSystem::destroy(audioSystem);
    RenderSystem::destroy(renderSystem);
    RDevice::destroy(renderDevice);
    AssetManager::destroy();
    WindowRegistry::destroy();
}

void RuntimeContextObj::render_frame(const Vec2& windowExtent)
{
    const Viewport windowViewport = Viewport::from_extent(windowExtent);

    ProjectSettings projectS = projectCtx.project().settings();

    // begin rendering a frame
    RenderSystemFrameInfo frameI{};
    frameI.directionalLight = Vec3(0.0f, 1.0f, 0.0f);
    frameI.screenExtent = windowExtent;
    frameI.sceneExtent = windowExtent;
    frameI.envCubemap = (RUID)0;
    frameI.clearColor = projectS.rendering_settings().get_clear_color();
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

    Vector<Viewport> screenViewports;
    Vector<Rect> aabbs;
    scene.get_screen_regions(screenViewports, aabbs);
    Vector<RenderSystemScreenPass::Region> regions(screenViewports.size());
    for (size_t i = 0; i < regions.size(); i++)
    {
        regions[i].viewport = screenViewports[i];
        regions[i].worldAABB = aabbs[i];
    }

    RenderSystemScreenPass screenP{};
    screenP.overlay.renderCallback = [](ScreenRenderComponent renderer, TView<int>, int overlayVPIndex, void* user) {
        RuntimeContextObj& self = *(RuntimeContextObj*)user;
        renderer.bind_quad_pipeline(QUAD_PIPELINE_UBER);
        renderer.set_view_projection_index(overlayVPIndex);
        self.scene.render_screen_ui(renderer);
    };

    screenP.overlay.viewport = windowViewport;
    screenP.regionCount = (uint32_t)regions.size();
    screenP.regions = regions.data();
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

    std::string err;
    if (!obj->startup(info, err))
    {
        heap_delete<RuntimeContextObj>(obj);
        return {};
    }

    return RuntimeContext(obj);
}

void RuntimeContext::destroy(RuntimeContext ctx)
{
    LD_PROFILE_SCOPE;

    auto* obj = (RuntimeContextObj*)ctx.unwrap();

    obj->cleanup();

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