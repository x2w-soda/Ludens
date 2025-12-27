#include <Ludens/Camera/Camera.h>
#include <Ludens/Header/Assert.h>
#include <Ludens/Header/Math/Math.h>
#include <Ludens/Lua/LuaModule.h>
#include <Ludens/Lua/LuaState.h>
#include <Ludens/Profiler/Profiler.h>
#include <Ludens/Scene/Scene.h>
#include <Ludens/System/Memory.h>

#include <iostream>
#include <vector>

#include "AudioServerCache.h"
#include "LuaScript.h"
#include "RenderServerCache.h"
#include "ScreenRenderer.h"
#include "SceneObj.h"

namespace LD {

/// @brief Scene Singleton, all scene operations including transition should be done in-place,
///        the SceneObj address should be immutable.
SceneObj* sScene = nullptr;

static void load_audio_source_component(SceneObj* scene, ComponentBase* base, void* comp);
static void unload_audio_source_component(SceneObj* scene, ComponentBase* base, void* comp);
static void cleanup_audio_source_component(SceneObj* scene, ComponentBase* base, void* comp);
static void startup_camera_component(SceneObj* scene, ComponentBase* base, void* comp);
static void cleanup_camera_component(SceneObj* scene, ComponentBase* base, void* comp);
static void load_mesh_component(SceneObj* scene, ComponentBase* base, void* comp);
static void load_sprite_2d_component(SceneObj* scene, ComponentBase* base, void* comp);

/// @brief Component behavior and operations within a Scene.
struct SceneComponent
{
    ComponentType type;
    void (*load)(SceneObj* scene, ComponentBase* base, void* comp);
    void (*unload)(SceneObj* scene, ComponentBase* base, void* comp);
    void (*startup)(SceneObj* scene, ComponentBase* base, void* comp);
    void (*cleanup)(SceneObj* scene, ComponentBase* base, void* comp);
};

// clang-format off
static SceneComponent sSceneComponents[] = {
    {COMPONENT_TYPE_DATA,          nullptr,                      nullptr,                        nullptr,                    nullptr},
    {COMPONENT_TYPE_AUDIO_SOURCE,  &load_audio_source_component, &unload_audio_source_component, nullptr,                    &cleanup_audio_source_component},
    {COMPONENT_TYPE_TRANSFORM,     nullptr,                      nullptr,                        nullptr,                    nullptr},
    {COMPONENT_TYPE_CAMERA,        nullptr,                      nullptr,                        &startup_camera_component,  &cleanup_camera_component},
    {COMPONENT_TYPE_MESH,          &load_mesh_component,         nullptr,                        nullptr,                    nullptr},
    {COMPONENT_TYPE_SPRITE_2D,     &load_sprite_2d_component,    nullptr,                        nullptr,                    nullptr},
};
// clang-format on

static_assert(sizeof(sSceneComponents) / sizeof(*sSceneComponents) == COMPONENT_TYPE_ENUM_COUNT);

//
// IMPLEMENTATION
//

static void load_audio_source_component(SceneObj* scene, ComponentBase* base, void* comp)
{
    AudioSourceComponent* sourceC = (AudioSourceComponent*)comp;

    if (sourceC->clipAUID)
    {
        AudioClipAsset clipA = (AudioClipAsset)scene->assetManager.get_asset(sourceC->clipAUID, ASSET_TYPE_AUDIO_CLIP);

        // NOTE: Buffer not destroyed upon component unload.
        //       Other components may still be using it for playback.
        AudioBuffer buffer = scene->audioServerCache.get_or_create_audio_buffer(clipA);

        if (buffer)
        {
            sourceC->playback = scene->audioServer.create_playback(buffer, sourceC->pan, sourceC->volumeLinear);
        }
    }
}

static void unload_audio_source_component(SceneObj* scene, ComponentBase* base, void* comp)
{
    AudioSourceComponent* sourceC = (AudioSourceComponent*)comp;

    if (sourceC->playback)
    {
        scene->audioServer.destroy_playback(sourceC->playback);
    }
}

static void cleanup_audio_source_component(SceneObj* scene, ComponentBase* base, void* comp)
{
    AudioSourceComponent* sourceC = (AudioSourceComponent*)comp;

    if (sourceC->playback)
    {
        scene->audioServer.stop_playback(sourceC->playback);
    }
}

static void startup_camera_component(SceneObj* scene, ComponentBase* base, void* comp)
{
    CameraComponent* cameraC = (CameraComponent*)comp;

    if (scene->mainCameraC)
    {
        LD_ASSERT(!cameraC->isMainCamera); // only one main camera allowed
        return;
    }

    scene->mainCameraC = cameraC;
    scene->mainCameraCUID = base->id;

    const Vec3 mainCameraTarget(0.0f, 0.0f, 1.0f);

    if (cameraC->isPerspective)
    {
        CameraPerspectiveInfo perspectiveI = cameraC->perspective;
        perspectiveI.aspectRatio = 1.0f; // updated per frame

        scene->mainCameraC->camera = Camera::create(perspectiveI, mainCameraTarget);
    }
    else
    {
        scene->mainCameraC->camera = Camera::create(cameraC->orthographic, mainCameraTarget);
    }
}

static void cleanup_camera_component(SceneObj* scene, ComponentBase* base, void* comp)
{
    if (scene->mainCameraC && scene->mainCameraCUID == base->id)
    {
        Camera::destroy(scene->mainCameraC->camera);
        scene->mainCameraC = nullptr;
        scene->mainCameraCUID = 0;
    }
}

static void load_mesh_component(SceneObj* scene, ComponentBase* base, void* comp)
{
    MeshComponent* meshC = (MeshComponent*)comp;

    if (meshC->auid)
    {
        RUID mesh = scene->renderServerCache.get_or_create_mesh(meshC->auid);
        scene->renderServerCache.create_mesh_draw_call(mesh, base->id);
    }
}

static void load_sprite_2d_component(SceneObj* scene, ComponentBase* base, void* comp)
{
    auto* spriteC = (Sprite2DComponent*)comp;

    if (spriteC->auid)
    {
        spriteC->image = scene->renderServerCache.get_or_create_image(spriteC->auid);
    }
}

void SceneObj::load(ComponentBase* base)
{
    LD_PROFILE_SCOPE;

    // polymorphic loading
    ComponentType type;
    void* comp = registry.get_component(base->id, &type);
    LD_ASSERT(type == base->type);

    if (sSceneComponents[(int)type].load)
        sSceneComponents[(int)type].load(this, base, comp);

    for (ComponentBase* child = base->child; child; child = child->next)
    {
        load(child);
    }
}

void SceneObj::unload(ComponentBase* base)
{
    LD_PROFILE_SCOPE;

    // polymorphic unloading
    ComponentType type;
    void* comp = registry.get_component(base->id, &type);
    LD_ASSERT(type == base->type);

    if (sSceneComponents[(int)type].unload)
        sSceneComponents[(int)type].unload(this, base, comp);

    for (ComponentBase* child = base->child; child; child = child->next)
    {
        unload(child);
    }
}

void SceneObj::startup_root(CUID root)
{
    ComponentBase* rootC = registry.get_component_base(root);

    if (!rootC)
        return;

    for (ComponentBase* childC = rootC->child; childC; childC = childC->next)
    {
        startup_root(childC->id);
    }

    // post-order traversal, all child components of root already have their scripts attached
    ComponentType type;
    void* comp = registry.get_component(rootC->id, &type);
    LD_ASSERT(type == rootC->type);

    if (sSceneComponents[(int)type].startup)
        sSceneComponents[(int)type].startup(this, rootC, comp);

    ComponentScriptSlot* script = registry.get_component_script(rootC->id);
    bool success = luaContext.create_lua_script(script);
    // TODO: abort startup at the first failure of creating lua script instance.
    luaContext.attach_lua_script(script);
}

void SceneObj::cleanup_root(CUID root)
{
    ComponentBase* rootC = registry.get_component_base(root);

    if (!rootC)
        return;

    for (ComponentBase* childC = rootC->child; childC; childC = childC->next)
    {
        cleanup_root(childC->id);
    }

    // post-order traversal, all child components of root already have their scripts detached
    ComponentType type;
    void* comp = registry.get_component(rootC->id, &type);
    LD_ASSERT(type == rootC->type);

    if (sSceneComponents[(int)type].cleanup)
        sSceneComponents[(int)type].cleanup(this, rootC, comp);

    ComponentScriptSlot* script = registry.get_component_script(rootC->id);
    luaContext.detach_lua_script(script);
    luaContext.destroy_lua_script(script);
}

//
// PUBLIC API
//

const char* get_lua_script_log_channel_name()
{
    return LuaScript::get_log_channel_name();
}

Scene Scene::create(const SceneInfo& sceneI)
{
    LD_PROFILE_SCOPE;

    LD_ASSERT(sScene == nullptr);
    sScene = heap_new<SceneObj>(MEMORY_USAGE_SCENE);
    sScene->registry = DataRegistry::create();
    sScene->screenRenderer.startup();
    sScene->assetManager = sceneI.assetManager;
    sScene->renderServer = sceneI.renderServer;
    sScene->renderServerCache.startup(sceneI.renderServer, sceneI.assetManager);
    sScene->audioServer = sceneI.audioServer;
    sScene->audioServerCache.startup(sceneI.audioServer);

    return Scene(sScene);
}

void Scene::destroy(Scene scene)
{
    LD_PROFILE_SCOPE;

    LD_ASSERT(sScene && sScene == scene.unwrap());

    // destroy all components
    scene.reset();

    sScene->audioServerCache.cleanup();
    sScene->renderServerCache.cleanup();
    sScene->screenRenderer.cleanup();
    DataRegistry::destroy(sScene->registry);

    heap_delete<SceneObj>(sScene);
    sScene = nullptr;
}

void Scene::reset()
{
    LD_PROFILE_SCOPE;

    if (mObj->state == SCENE_STATE_LOADED)
        unload();

    LD_ASSERT(mObj->state == SCENE_STATE_EMPTY);

    if (mObj->registryBack)
    {
        DataRegistry::destroy(mObj->registryBack);
        mObj->registryBack = {};
    }

    if (mObj->registry)
    {
        DataRegistry::destroy(mObj->registry);
        mObj->registry = {};
    }

    if (mObj->mainCameraC)
    {
        Camera::destroy(mObj->mainCameraC->camera);
        mObj->mainCameraC = nullptr;
    }

    mObj->mainCameraCUID = 0;
    mObj->screenRenderer.cleanup();
    mObj->screenRenderer.startup();
    mObj->registry = DataRegistry::create();
}

void Scene::load()
{
    LD_PROFILE_SCOPE;

    if (mObj->state == SCENE_STATE_LOADED)
    {
        LD_UNREACHABLE;
        return;
    }

    mObj->state = SCENE_STATE_LOADED;
    mObj->luaContext.startup(*this, mObj->registry, mObj->assetManager);

    std::vector<CUID> roots;
    mObj->registry.get_root_components(roots);

    for (CUID rootID : roots)
    {
        ComponentBase* base = mObj->registry.get_component_base(rootID);
        mObj->load(base);
    }
}

void Scene::unload()
{
    LD_PROFILE_SCOPE;

    if (mObj->state != SCENE_STATE_LOADED)
        return;

    std::vector<CUID> roots;
    mObj->registry.get_root_components(roots);

    for (CUID rootID : roots)
    {
        ComponentBase* base = mObj->registry.get_component_base(rootID);
        mObj->unload(base);
    }

    mObj->luaContext.cleanup();
    mObj->state = SCENE_STATE_EMPTY;
}

void Scene::startup()
{
    LD_PROFILE_SCOPE;

    if (mObj->state == SCENE_STATE_RUNNING)
        return;

    mObj->state = SCENE_STATE_RUNNING;
    mObj->luaContext.set_registry(mObj->registry);

    std::vector<CUID> roots;
    mObj->registry.get_root_components(roots);

    for (CUID root : roots)
    {
        mObj->startup_root(root);
    }
}

void Scene::cleanup()
{
    LD_PROFILE_SCOPE;

    if (mObj->state != SCENE_STATE_RUNNING)
        return;

    mObj->state = SCENE_STATE_LOADED;

    std::vector<CUID> roots;
    mObj->registry.get_root_components(roots);

    for (CUID root : roots)
    {
        mObj->cleanup_root(root);
    }

    mObj->mainCameraC = nullptr;
    mObj->mainCameraCUID = (CUID)0;
}

void Scene::backup()
{
    LD_PROFILE_SCOPE;

    if (mObj->state != SCENE_STATE_LOADED)
        return;

    if (mObj->registryBack)
        DataRegistry::destroy(mObj->registryBack);

    mObj->registryBack = mObj->registry.duplicate();
}

void Scene::swap()
{
    if (mObj->state != SCENE_STATE_LOADED)
        return;

    DataRegistry tmp = mObj->registry;
    mObj->registry = mObj->registryBack;
    mObj->registryBack = tmp;
}

void Scene::update(const Vec2& screenExtent, float delta)
{
    LD_PROFILE_SCOPE;
    LD_ASSERT(screenExtent.x > 0.0f && screenExtent.y > 0.0f);

    mObj->screenExtent = screenExtent;

    // update all lua script instances
    mObj->luaContext.update(delta);

    if (mObj->mainCameraC)
    {
        const CameraComponent* cameraC = mObj->mainCameraC;
        Camera mainCamera = mObj->mainCameraC->camera;
        Mat4 worldTransform;
        mObj->registry.get_component_transform_mat4(mObj->mainCameraCUID, worldTransform);
        Vec3 forward = worldTransform.as_mat3() * Vec3(0.0f, 0.0f, 1.0f);

        mainCamera.set_aspect_ratio(screenExtent.x / screenExtent.y);
        mainCamera.set_pos(cameraC->transform.position);
        mainCamera.set_target(cameraC->transform.position + forward);
    }

    // update screen layer information
    mObj->screenRenderer.render(mObj->registry);

    // any heap allocations for audio is done on main thread.
    mObj->audioServer.update();
}

Camera Scene::get_camera()
{
    if (mObj->mainCameraC)
        return mObj->mainCameraC->camera;

    return {};
}

ScreenLayer Scene::get_screen_layer()
{
    return mObj->screenRenderer.get_layer();
}

CUID Scene::create_component(ComponentType type, const char* name, CUID parent, CUID hint)
{
    return mObj->registry.create_component(type, name, parent, hint);
}

ComponentScriptSlot* Scene::create_component_script_slot(CUID compID, AUID assetID)
{
    return mObj->registry.create_component_script_slot(compID, assetID);
}

void Scene::destroy_component_script_slot(CUID compID)
{
    mObj->registry.destroy_component_script_slot(compID);
}

void Scene::destroy_component(CUID compID)
{
    mObj->registry.destroy_component(compID);
}

void Scene::reparent(CUID compID, CUID parentID)
{
    mObj->registry.reparent(compID, parentID);
}

void Scene::get_root_components(std::vector<CUID>& roots)
{
    mObj->registry.get_root_components(roots);
}

ComponentBase* Scene::get_component_base(CUID compID)
{
    return mObj->registry.get_component_base(compID);
}

ComponentScriptSlot* Scene::get_component_script_slot(CUID compID)
{
    return mObj->registry.get_component_script(compID);
}

void* Scene::get_component(CUID compID, ComponentType* outType)
{
    return mObj->registry.get_component(compID, outType);
}

void* Scene::get_component(CUID compID, ComponentType expectedType)
{
    ComponentType foundType;
    void* comp = mObj->registry.get_component(compID, &foundType);

    if (!comp || foundType != expectedType)
        return nullptr;

    return comp;
}

RUID Scene::get_component_ruid(CUID compID)
{
    return mObj->renderServerCache.get_component_ruid(compID);
}

bool Scene::get_component_transform(CUID compID, TransformEx& transform)
{
    return mObj->registry.get_component_transform(compID, transform);
}

bool Scene::set_component_transform(CUID compID, const TransformEx& transform)
{
    return mObj->registry.set_component_transform(compID, transform);
}

bool Scene::get_component_transform2d(CUID compID, Transform2D& transform)
{
    return mObj->registry.get_component_transform2d(compID, transform);
}

bool Scene::get_component_transform_mat4(CUID compID, Mat4& worldMat4)
{
    return mObj->registry.get_component_transform_mat4(compID, worldMat4);
}

void Scene::mark_component_transform_dirty(CUID compID)
{
    mObj->registry.mark_component_transform_dirty(compID);
}

CUID Scene::get_ruid_component(RUID ruid)
{
    return mObj->renderServerCache.get_ruid_component(ruid);
}

Mat4 Scene::get_ruid_transform_mat4(RUID ruid)
{
    CUID compID = get_ruid_component(ruid);

    Mat4 worldMat4;
    mObj->registry.get_component_transform_mat4(compID, worldMat4);

    return worldMat4;
}

Scene::IAudioSource::IAudioSource(AudioSourceComponent* comp)
    : mComp(comp)
{
    LD_ASSERT(mComp);
}

void Scene::IAudioSource::play()
{
    if (!mComp)
        return;

    sScene->audioServer.start_playback(mComp->playback);
}

void Scene::IAudioSource::pause()
{
    if (!mComp)
        return;

    sScene->audioServer.pause_playback(mComp->playback);
}

void Scene::IAudioSource::resume()
{
    if (!mComp)
        return;

    sScene->audioServer.resume_playback(mComp->playback);
}

void Scene::IAudioSource::set_clip_asset(AUID clipAUID)
{
    if (!mComp)
        return;

    AudioClipAsset clipA(sScene->assetManager.get_asset(clipAUID).unwrap());
    AudioBuffer buffer = sScene->audioServerCache.get_or_create_audio_buffer(clipA);

    if (buffer)
    {
        mComp->clipAUID = clipAUID;
        sScene->audioServer.set_playback_buffer(mComp->playback, buffer);
    }
}

Scene::IMesh::IMesh(CUID meshCUID)
    :  mCUID(meshCUID)
{
    mComp = (MeshComponent*)Scene(sScene).get_component(meshCUID, COMPONENT_TYPE_MESH);
    LD_ASSERT(mComp);
}

void Scene::IMesh::set_mesh_asset(AUID meshAUID)
{
    RUID meshID = sScene->renderServerCache.get_mesh(meshAUID);
    LD_ASSERT(meshID != 0);

    mComp->auid = meshAUID;
    sScene->renderServerCache.create_mesh_draw_call(meshID, mCUID);
}

Scene::ISprite2D::ISprite2D(Sprite2DComponent* comp)
    : mComp(comp)
{
    LD_ASSERT(mComp);
}

void Scene::ISprite2D::set_texture_2d_asset(AUID textureAUID)
{
    mComp->auid = textureAUID;
    mComp->image = sScene->renderServerCache.get_or_create_image(textureAUID);
}

} // namespace LD