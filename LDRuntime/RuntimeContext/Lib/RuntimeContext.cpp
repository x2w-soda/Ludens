#include <Ludens/Asset/AssetType/FontAsset.h>
#include <Ludens/Asset/AssetType/TextureCubeAsset.h>
#include <Ludens/AudioServer/AudioServer.h>
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
    RenderServer renderServer;
    AudioServer audioServer;
    AssetManager AM;
    Project project;
    Scene scene;
    RUID envCubemapID = 0;

    void render_frame(const Vec2& screenExtent);

    /// @brief Callback to inform the render server the transforms of RUIDs.
    static Mat4 render_server_transform_callback(RUID ruid, void* user);
};

void RuntimeContextObj::render_frame(const Vec2& screenExtent)
{
    // begin rendering a frame
    RenderServerFrameInfo frameI{};
    frameI.directionalLight = Vec3(0.0f, 1.0f, 0.0f);
    frameI.mainCamera = scene.get_camera();
    frameI.screenExtent = screenExtent;
    frameI.sceneExtent = screenExtent;
    frameI.envCubemap = envCubemapID;
    frameI.clearColor = project.get_settings().get_rendering_settings().get_clear_color();
    renderServer.next_frame(frameI);

    // render game scene without editor overlay
    RenderServerScenePass sceneP{};
    sceneP.transformCallback = &RuntimeContextObj::render_server_transform_callback;
    sceneP.user = this;
    sceneP.overlay.enabled = false;
    sceneP.hasSkybox = envCubemapID != 0;
    renderServer.scene_pass(sceneP);

    renderServer.submit_frame();
}

Mat4 RuntimeContextObj::render_server_transform_callback(RUID ruid, void* user)
{
    RuntimeContextObj& self = *(RuntimeContextObj*)user;

    return self.scene.get_ruid_transform_mat4(ruid);
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
        deviceI.vsync = true; // TODO expose
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
    RenderServerInfo serverI{};
    serverI.device = obj->renderDevice;
    serverI.fontAtlas = defaultFont.get_font_atlas();
    obj->renderServer = RenderServer::create(serverI);
    obj->audioServer = AudioServer::create();

    obj->envCubemapID = obj->renderServer.cubemap().create_data_id(textureCubeAsset.get_bitmap());

    SceneInfo sceneI{};
    sceneI.assetManager = obj->AM;
    sceneI.audioServer = obj->audioServer;
    sceneI.renderServer = obj->renderServer;
    obj->scene = Scene::create(sceneI);

    // load default scene
    std::string err;
    bool ok = SceneSchema::load_scene_from_file(obj->scene, defaultScenePath, err);
    LD_ASSERT(ok); // TODO:
    obj->scene.load();
    obj->scene.startup();

    return RuntimeContext(obj);
}

void RuntimeContext::destroy(RuntimeContext ctx)
{
    LD_PROFILE_SCOPE;

    auto* obj = (RuntimeContextObj*)ctx.unwrap();
    obj->scene.cleanup();

    Scene::destroy(obj->scene);
    AudioServer::destroy(obj->audioServer);
    RenderServer::destroy(obj->renderServer);
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