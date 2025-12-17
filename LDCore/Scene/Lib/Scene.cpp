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

#include "LuaScript.h"

namespace LD {

enum SceneState
{
    SCENE_STATE_EMPTY = 0,
    SCENE_STATE_LOADED,
    SCENE_STATE_RUNNING,
};

/// @brief Scene implementation.
struct SceneObj
{
    DataRegistry registry;
    DataRegistry registryBack;
    AssetManager assetManager{};
    AudioServer audioServer{};
    RenderServer renderServer{};
    LuaScript::Context luaContext{};
    CameraComponent* mainCameraC;
    CUID mainCameraCUID;
    Vec2 screenExtent = {};
    std::unordered_map<RUID, CUID> ruidToCuid;          /// map draw call to corresponding component
    std::unordered_map<CUID, RUID> cuidToRuid;          /// map component to corresponding draw call
    std::unordered_map<AUID, RUID> auidToRuid;          /// map asset to GPU resource
    std::unordered_map<AUID, AudioBuffer> clipToBuffer; /// map audio clip to audio buffer
    SceneState state = SCENE_STATE_EMPTY;

    /// @brief Load components recursively, creating resources from systems/servers.
    void load(ComponentBase* comp);

    /// @brief Unload components recursively, destroying resources from systems/servers.
    void unload(ComponentBase* comp);

    /// @brief Startup a component subtree recursively, attaching scripts to components
    void startup_root(CUID compID);

    /// @brief Cleanup a component subtree recursively, detaching scripts from components
    void cleanup_root(CUID compID);

    /// @brief Get or create corresponding audio buffer from asset.
    AudioBuffer get_or_create_audio_buffer(AudioClipAsset clipA);
};

static void load_audio_source_component(SceneObj* scene, ComponentBase* base, void* comp);
static void unload_audio_source_component(SceneObj* scene, ComponentBase* base, void* comp);
static void cleanup_audio_source_component(SceneObj* scene, ComponentBase* base, void* comp);
static void startup_camera_component(SceneObj* scene, ComponentBase* base, void* comp);
static void cleanup_camera_component(SceneObj* scene, ComponentBase* base, void* comp);
static void load_mesh_component(SceneObj* scene, ComponentBase* base, void* comp);

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
    {COMPONENT_TYPE_SPRITE_2D,     nullptr,                      nullptr,                        nullptr,                    nullptr},
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
        AudioClipAsset clipA(scene->assetManager.get_asset(sourceC->clipAUID).unwrap());

        // NOTE: Buffer not destroyed upon component unload.
        //       Other components may still be using it for playback.
        AudioBuffer buffer = scene->get_or_create_audio_buffer(clipA);

        if (buffer)
        {
            sourceC->playback = scene->audioServer.create_playback(buffer);
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
        MeshAsset meshA(scene->assetManager.get_asset(meshC->auid).unwrap());

        if (!scene->auidToRuid.contains(meshA.get_auid()))
            scene->auidToRuid[meshC->auid] = scene->renderServer.create_mesh(*meshA.data());

        RUID mesh = scene->auidToRuid[meshC->auid];
        RUID drawCall = scene->renderServer.create_mesh_draw_call(mesh);
        scene->ruidToCuid[drawCall] = base->id;
        scene->cuidToRuid[base->id] = drawCall;
    }
}

void SceneObj::load(ComponentBase* base)
{
    LD_PROFILE_SCOPE;

    // polymorphic loading
    ComponentType type;
    void* comp = registry.get_component(base->id, type);
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
    void* comp = registry.get_component(base->id, type);
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
    void* comp = registry.get_component(rootC->id, type);
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
    void* comp = registry.get_component(rootC->id, type);
    LD_ASSERT(type == rootC->type);

    if (sSceneComponents[(int)type].cleanup)
        sSceneComponents[(int)type].cleanup(this, rootC, comp);

    ComponentScriptSlot* script = registry.get_component_script(rootC->id);
    luaContext.detach_lua_script(script);
    luaContext.destroy_lua_script(script);
}

AudioBuffer SceneObj::get_or_create_audio_buffer(AudioClipAsset clipA)
{
    if (!clipA)
        return {};

    AUID clipAUID = clipA.get_auid();

    if (clipToBuffer.contains(clipAUID))
        return clipToBuffer[clipAUID];

    AudioBufferInfo bufferI{};
    bufferI.channels = clipA.get_channel_count();
    bufferI.format = SAMPLE_FORMAT_F32;
    bufferI.frameCount = clipA.get_frame_count();
    bufferI.sampleRate = clipA.get_sample_rate();
    bufferI.samples = clipA.get_frames(0);
    AudioBuffer buffer = audioServer.create_buffer(bufferI);

    if (buffer)
        clipToBuffer[clipAUID] = buffer;

    return buffer;
}

//
// PUBLIC API
//

const char* get_lua_script_log_channel_name()
{
    return LuaScript::get_log_channel_name();
}

Scene Scene::create()
{
    LD_PROFILE_SCOPE;

    SceneObj* obj = heap_new<SceneObj>(MEMORY_USAGE_SCENE);
    obj->registry = DataRegistry::create();
    obj->assetManager = {};
    obj->renderServer = {};
    obj->audioServer = {};

    return Scene(obj);
}

void Scene::destroy(Scene scene)
{
    LD_PROFILE_SCOPE;

    SceneObj* obj = scene.unwrap();

    if (obj->state == SCENE_STATE_LOADED)
        scene.unload();

    LD_ASSERT(obj->state == SCENE_STATE_EMPTY);

    // All playbacks should have been destroyed
    for (auto ite : obj->clipToBuffer)
    {
        AudioBuffer buffer = ite.second;
        obj->audioServer.destroy_buffer(buffer);
    }
    obj->clipToBuffer.clear();

    if (obj->registryBack)
        DataRegistry::destroy(obj->registryBack);

    if (obj->mainCameraC)
        Camera::destroy(obj->mainCameraC->camera);

    obj->mainCameraCUID = 0;

    DataRegistry::destroy(obj->registry);

    heap_delete<SceneObj>(obj);
}

void Scene::load(const SceneLoadInfo& info)
{
    LD_PROFILE_SCOPE;

    if (mObj->state == SCENE_STATE_LOADED)
    {
        LD_UNREACHABLE;
        return;
    }

    mObj->state = SCENE_STATE_LOADED;

    LD_ASSERT(info.assetManager && info.renderServer && info.audioServer);
    mObj->assetManager = info.assetManager;
    mObj->renderServer = info.renderServer;
    mObj->audioServer = info.audioServer;
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

    mObj->state = SCENE_STATE_EMPTY;

    std::vector<CUID> roots;
    mObj->registry.get_root_components(roots);

    for (CUID rootID : roots)
    {
        ComponentBase* base = mObj->registry.get_component_base(rootID);
        mObj->unload(base);
    }

    mObj->luaContext.cleanup();
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

    mObj->audioServer.update();
}

Camera Scene::get_camera()
{
    if (mObj->mainCameraC)
        return mObj->mainCameraC->camera;

    return {};
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

void* Scene::get_component(CUID compID, ComponentType& type)
{
    return mObj->registry.get_component(compID, type);
}

RUID Scene::get_component_ruid(CUID compID)
{
    auto ite = mObj->cuidToRuid.find(compID);

    if (ite == mObj->cuidToRuid.end())
        return 0;

    return ite->second;
}

bool Scene::get_component_transform(CUID compID, Transform& transform)
{
    return mObj->registry.get_component_transform(compID, transform);
}

bool Scene::set_component_transform(CUID compID, const Transform& transform)
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
    auto ite = mObj->ruidToCuid.find(ruid);

    if (ite == mObj->ruidToCuid.end())
        return 0;

    return ite->second;
}

Mat4 Scene::get_ruid_transform_mat4(RUID ruid)
{
    CUID compID = get_ruid_component(ruid);

    Mat4 worldMat4;
    mObj->registry.get_component_transform_mat4(compID, worldMat4);

    return worldMat4;
}

Scene::IAudioSource::IAudioSource(Scene scene, CUID sourceCUID)
    : mScene(scene.unwrap())
{
    ComponentType type;
    mComp = (AudioSourceComponent*)scene.get_component(sourceCUID, type);
    LD_ASSERT(type == COMPONENT_TYPE_AUDIO_SOURCE);
}

void Scene::IAudioSource::play()
{
    if (!mComp)
        return;

    mScene->audioServer.start_playback(mComp->playback);
}

void Scene::IAudioSource::pause()
{
    if (!mComp)
        return;

    mScene->audioServer.pause_playback(mComp->playback);
}

void Scene::IAudioSource::resume()
{
    if (!mComp)
        return;

    mScene->audioServer.resume_playback(mComp->playback);
}

void Scene::IAudioSource::set_clip_asset(AUID clipAUID)
{
    if (!mComp)
        return;

    AudioClipAsset clipA(mScene->assetManager.get_asset(clipAUID).unwrap());
    AudioBuffer buffer = mScene->get_or_create_audio_buffer(clipA);

    if (buffer)
    {
        mComp->clipAUID = clipAUID;
        mScene->audioServer.set_playback_buffer(mComp->playback, buffer);
    }
}

Scene::IMesh::IMesh(Scene scene, CUID meshCUID)
    : mScene(scene.unwrap()), mCUID(meshCUID)
{
    ComponentType type;
    mComp = (MeshComponent*)scene.get_component(meshCUID, type);
    LD_ASSERT(type == COMPONENT_TYPE_MESH);
}

void Scene::IMesh::set_mesh_asset(AUID meshAUID)
{
    if (!mScene->auidToRuid.contains(meshAUID))
        return;

    RUID mesh = mScene->auidToRuid[meshAUID];
    if (!mScene->renderServer.mesh_exists(mesh))
        return;

    mComp->auid = meshAUID;

    auto ite = mScene->cuidToRuid.find(mCUID);
    if (ite != mScene->cuidToRuid.end())
    {
        RUID oldDrawCall = ite->second;
        mScene->renderServer.destroy_mesh_draw_call(oldDrawCall);
        mScene->ruidToCuid.erase(oldDrawCall);
        mScene->cuidToRuid.erase(mCUID);
    }

    RUID drawCall = mScene->renderServer.create_mesh_draw_call(mesh);
    mScene->cuidToRuid[mCUID] = drawCall;
    mScene->ruidToCuid[drawCall] = mCUID;
}

} // namespace LD