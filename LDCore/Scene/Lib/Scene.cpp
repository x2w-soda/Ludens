#include "LuaScript.h"
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
    RServer renderServer{};
    LuaState lua;
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

    /// @brief Create lua script associated with a component.
    void create_lua_script(ComponentScriptSlot* scriptSlot);

    /// @brief Destroy lua script associated with a component
    void destroy_lua_script(ComponentScriptSlot* scriptSlot);

    /// @brief Attach lua script to a data component.
    void attach_lua_script(ComponentScriptSlot* scriptSlot);

    /// @brief Detach lua script from a data component.
    void detach_lua_script(ComponentScriptSlot* scriptSlot);

    /// @brief Initialize a lua state for scripting
    void initialize_lua_state(LuaState L);

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
        AudioClipAsset clipA = scene->assetManager.get_audio_clip_asset(sourceC->clipAUID);

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
        MeshAsset meshA = scene->assetManager.get_mesh_asset(meshC->auid);

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
    create_lua_script(script);
    attach_lua_script(script);
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
    detach_lua_script(script);
    destroy_lua_script(script);
}

void SceneObj::create_lua_script(ComponentScriptSlot* scriptSlot)
{
    if (!scriptSlot)
        return;

    int oldSize = lua.size();

    CUID compID = scriptSlot->componentID;
    AUID assetID = scriptSlot->assetID;

    lua.get_global("ludens");
    lua.get_field(-1, "scripts");
    lua.push_number((double)compID);

    LuaScriptAsset asset = assetManager.get_lua_script_asset(assetID);
    LD_ASSERT(asset);
    const char* luaSource = asset.get_source();

    // this should push the script instance table onto stack
    bool isScriptValid = lua.do_string(luaSource);
    if (!isScriptValid)
    {
        printf("Error: %s\n", lua.to_string(-1));
    }

    LD_ASSERT(isScriptValid); // TODO: error control flow
    lua.set_table(-3);        // store script instance as ludens.scripts[compID]

    ComponentType type;
    void* comp = registry.get_component(compID, type);

    // create and store table for component type
    LuaScript::create_component_table(Scene(this), registry, lua, compID, type, comp);

    lua.resize(oldSize);
}

void SceneObj::destroy_lua_script(ComponentScriptSlot* scriptSlot)
{
    if (!scriptSlot)
        return;

    CUID compID = scriptSlot->componentID;
    int oldSize = lua.size();
    lua.get_global("ludens");
    lua.get_field(-1, "scripts");

    // destroy component lua table representation
    LuaScript::destroy_component_table(Scene(this), registry, lua, compID);

    lua.push_number((double)compID);
    lua.push_nil();
    lua.set_table(-3);

    lua.resize(oldSize);
}

// Caller should prepare ludens.scripts table on top of stack
void SceneObj::attach_lua_script(ComponentScriptSlot* scriptSlot)
{
    if (!scriptSlot)
        return;

    LD_ASSERT(lua.get_type(-1) == LUA_TYPE_TABLE);

    int oldSize = lua.size();
    CUID compID = scriptSlot->componentID;
    LuaType type;

    // call 'attach' lua method on script
    lua.push_number((double)compID);
    lua.get_table(-2);
    LD_ASSERT(lua.get_type(-1) == LUA_TYPE_TABLE); // script instance

    lua.get_field(-1, "attach");
    LD_ASSERT(lua.get_type(-1) == LUA_TYPE_FN); // script attach method

    // arg1 is script instance
    lua.push_value(-2);
    LD_ASSERT((type = lua.get_type(-1)) == LUA_TYPE_TABLE);

    // arg2 is the component
    lua.get_field(-3, "_comp");
    LD_ASSERT(lua.get_type(-1) == LUA_TYPE_TABLE);

    lua.call(2, 0);

    lua.resize(oldSize);
}

// Caller should prepare ludens.scripts table on top of stack
void SceneObj::detach_lua_script(ComponentScriptSlot* scriptSlot)
{
    if (!scriptSlot)
        return;

    LD_ASSERT(lua.get_type(-1) == LUA_TYPE_TABLE);

    int oldSize = lua.size();
    CUID compID = scriptSlot->componentID;
    LuaType type;

    // call 'detach' lua method on script
    lua.push_number((double)compID);
    lua.get_table(-2);
    lua.get_field(-1, "detach");
    LD_ASSERT((type = lua.get_type(-1)) == LUA_TYPE_FN); // script detach method

    // arg1 is script instance
    lua.push_value(-2);
    LD_ASSERT((type = lua.get_type(-1)) == LUA_TYPE_TABLE);

    lua.call(1, 0);

    lua.resize(oldSize);
}

void SceneObj::initialize_lua_state(LuaState L)
{
    LuaModule ludensLuaModule = LuaScript::create_ludens_module();
    ludensLuaModule.load(L);
    LuaModule::destroy(ludensLuaModule);

    bool isModuleReady = L.do_string("_G.ludens = require 'ludens'");
    LD_ASSERT(isModuleReady);

    L.get_global("ludens");
    L.push_table();
    L.set_field(-2, "scripts");
    L.clear();
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

    // lua scripting context
    LuaStateInfo stateI{};
    stateI.openLibs = true;
    obj->lua = LuaState::create(stateI);
    obj->initialize_lua_state(obj->lua);

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
    LuaState::destroy(obj->lua);

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
}

void Scene::startup()
{
    LD_PROFILE_SCOPE;

    if (mObj->state == SCENE_STATE_RUNNING)
        return;

    mObj->state = SCENE_STATE_RUNNING;
    mObj->lua.clear();
    mObj->lua.get_global("ludens");
    mObj->lua.get_field(-1, "scripts");

    std::vector<CUID> roots;
    mObj->registry.get_root_components(roots);

    for (CUID root : roots)
    {
        mObj->startup_root(root);
    }

    mObj->lua.clear();
}

void Scene::cleanup()
{
    LD_PROFILE_SCOPE;

    if (mObj->state != SCENE_STATE_RUNNING)
        return;

    mObj->state = SCENE_STATE_LOADED;

    mObj->lua.get_global("ludens");
    mObj->lua.get_field(-1, "scripts");

    std::vector<CUID> roots;
    mObj->registry.get_root_components(roots);

    for (CUID root : roots)
    {
        mObj->cleanup_root(root);
    }

    mObj->lua.pop(2);

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

    LuaState L = mObj->lua;
    int oldSize1 = L.size();
    L.get_global("ludens");
    L.get_field(-1, "scripts");

    for (auto ite = mObj->registry.get_component_scripts(); ite; ++ite)
    {
        auto* script = (ComponentScriptSlot*)ite.data();
        if (!script->isEnabled)
            continue;

        int oldSize2 = L.size();
        L.push_number((double)script->componentID);
        L.get_table(-2);

        L.get_field(-1, "update");
        LD_ASSERT(L.get_type(-1) == LUA_TYPE_FN);

        // arg1 is the script instance (lua table)
        L.push_number((double)script->componentID);
        L.get_table(-4);

        // arg2 is the component (lua table) the script is attached to
        L.get_field(-1, "_comp");
        LD_ASSERT(L.get_type(-1) == LUA_TYPE_TABLE);

        // arg3 is the frame delta time
        L.push_number((double)delta);

        // Script:update(comp, delta)
        {
            LD_PROFILE_SCOPE_NAME("LuaScript pcall");
            LuaError err = L.pcall(3, 0, 0);
            LD_ASSERT(err == 0);
        }

        L.resize(oldSize2);
    }

    L.resize(oldSize1);

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

    AudioClipAsset clipA = mScene->assetManager.get_audio_clip_asset(clipAUID);
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