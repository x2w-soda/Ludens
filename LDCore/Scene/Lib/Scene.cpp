#include <Ludens/Camera/Camera.h>
#include <Ludens/DSA/Vector.h>
#include <Ludens/Header/Assert.h>
#include <Ludens/Header/Math/Math.h>
#include <Ludens/Lua/LuaModule.h>
#include <Ludens/Lua/LuaState.h>
#include <Ludens/Memory/Memory.h>
#include <Ludens/Profiler/Profiler.h>
#include <Ludens/Scene/Scene.h>

// Scene implementation may know about DataRegistry memory layouts.
#include <Ludens/DataRegistry/DataComponent.h>

#include <iostream>

#include "AudioSystemCache.h"
#include "LuaScript.h"
#include "RenderSystemCache.h"
#include "SceneObj.h"

// Scene user's responsibility to check handle before calling methods
#define LD_ASSERT_COMPONENT_LOADED LD_ASSERT(static_cast<bool>(*this))

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

    if (sourceC->clipID)
    {
        // NOTE: Buffer not destroyed upon component unload.
        //       Other components may still be using it for playback.
        AudioBuffer buffer = scene->audioSystemCache.get_or_create_audio_buffer(sourceC->clipID);

        if (buffer)
        {
            sourceC->playback = scene->audioSystemCache.create_playback(buffer, sourceC);
        }
    }
}

static void unload_audio_source_component(SceneObj* scene, ComponentBase* base, void* comp)
{
    AudioSourceComponent* sourceC = (AudioSourceComponent*)comp;

    if (sourceC->playback)
    {
        scene->audioSystemCache.destroy_playback(sourceC->playback);
    }
}

static void cleanup_audio_source_component(SceneObj* scene, ComponentBase* base, void* comp)
{
    AudioSourceComponent* sourceC = (AudioSourceComponent*)comp;

    if (sourceC->playback)
    {
        scene->audioSystemCache.stop_playback(sourceC->playback);
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
    scene->mainCameraCUID = base->cuid;

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
    if (scene->mainCameraC && scene->mainCameraCUID == base->cuid)
    {
        Camera::destroy(scene->mainCameraC->camera);
        scene->mainCameraC = nullptr;
        scene->mainCameraCUID = 0;
    }
}

static void load_mesh_component(SceneObj* scene, ComponentBase* base, void* comp)
{
    MeshComponent* meshC = (MeshComponent*)comp;
    meshC->draw = scene->renderSystemCache.create_mesh_draw(base->cuid, 0);

    if (meshC->assetID)
    {
        Scene::Mesh mesh(meshC);

        mesh.set_mesh_asset(meshC->assetID);
    }
}

static void load_sprite_2d_component(SceneObj* scene, ComponentBase* base, void* comp)
{
    auto* spriteC = (Sprite2DComponent*)comp;
    LD_UNREACHABLE; // create draw with valid screen layer

    if (spriteC->assetID)
    {
        Scene::Sprite2D sprite(spriteC);

        sprite.set_texture_2d_asset(spriteC->assetID);
    }
}

void SceneObj::load(ComponentBase* base)
{
    LD_PROFILE_SCOPE;

    // polymorphic loading
    ComponentType type;
    void* compData = registry.get_component_data(base->cuid, &type);
    LD_ASSERT(type == base->type);

    if (sSceneComponents[(int)type].load)
        sSceneComponents[(int)type].load(this, base, compData);

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
    void* compData = registry.get_component_data(base->cuid, &type);
    LD_ASSERT(type == base->type);

    if (sSceneComponents[(int)type].unload)
        sSceneComponents[(int)type].unload(this, base, compData);

    for (ComponentBase* child = base->child; child; child = child->next)
    {
        unload(child);
    }
}

void SceneObj::startup_root(CUID rootID)
{
    ComponentBase* rootC = registry.get_component_base(rootID);

    if (!rootC)
        return;

    for (ComponentBase* childC = rootC->child; childC; childC = childC->next)
    {
        startup_root(childC->cuid);
    }

    // post-order traversal, all child components of root already have their scripts attached
    ComponentType type;
    void* compData = registry.get_component_data(rootC->cuid, &type);
    LD_ASSERT(type == rootC->type);

    if (sSceneComponents[(int)type].startup)
        sSceneComponents[(int)type].startup(this, rootC, compData);

    if (rootC->scriptAssetID)
    {
        luaContext.create_component_table(rootC->cuid);
        bool success = luaContext.create_lua_script(rootID, rootC->scriptAssetID); // TODO: abort startup at the first failure of creating lua script instance.
        luaContext.attach_lua_script(rootID);
    }
}

void SceneObj::cleanup_root(CUID rootID)
{
    ComponentBase* rootC = registry.get_component_base(rootID);

    if (!rootC)
        return;

    for (ComponentBase* childC = rootC->child; childC; childC = childC->next)
    {
        cleanup_root(childC->cuid);
    }

    // post-order traversal, all child components of root already have their scripts detached
    ComponentType type;
    void* compData = registry.get_component_data(rootC->cuid, &type);
    LD_ASSERT(type == rootC->type);

    if (sSceneComponents[(int)type].cleanup)
        sSceneComponents[(int)type].cleanup(this, rootC, compData);

    luaContext.detach_lua_script(rootID);
    luaContext.destroy_lua_script(rootID);

    // component table may exist even if there is no script attached it
    luaContext.destroy_component_table(rootC->cuid);
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
    sScene->assetManager = sceneI.assetManager;
    sScene->renderSystemCache.startup(sceneI.renderSystem, sceneI.assetManager);
    sScene->audioSystemCache.startup(sceneI.audioSystem, sceneI.assetManager);

    return Scene(sScene);
}

void Scene::destroy(Scene scene)
{
    LD_PROFILE_SCOPE;

    LD_ASSERT(sScene && sScene == scene.unwrap());

    // destroy all components
    scene.reset();

    sScene->audioSystemCache.cleanup();
    sScene->renderSystemCache.cleanup();
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
        LD::Camera::destroy(mObj->mainCameraC->camera);
        mObj->mainCameraC = nullptr;
    }

    mObj->mainCameraCUID = 0;

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
        LD::Camera mainCamera = mObj->mainCameraC->camera;
        Mat4 worldMat4;
        mObj->registry.get_component_world_mat4(mObj->mainCameraCUID, worldMat4);
        Vec3 forward = worldMat4.as_mat3() * Vec3(0.0f, 0.0f, 1.0f);

        mainCamera.set_aspect_ratio(screenExtent.x / screenExtent.y);
        mainCamera.set_pos(cameraC->transform.position);
        mainCamera.set_target(cameraC->transform.position + forward);
    }

    // any heap allocations for audio is done on main thread.
    mObj->audioSystemCache.update();
}

Camera Scene::get_camera()
{
    if (mObj->mainCameraC)
        return mObj->mainCameraC->camera;

    return {};
}

Scene::Component Scene::create_component(ComponentType type, const char* name, CUID parentCUID)
{
    CUID compCUID = mObj->registry.create_component(type, name, parentCUID, (SUID)0);

    // TODO: DataRegistry API without CUID -> Component Data chasing.
    return Scene::Component(mObj->registry.get_component_data(compCUID, nullptr));
}

Scene::Component Scene::create_component_serial(ComponentType type, const char* name, SUID parentSUID, SUID hintSUID)
{
    ComponentBase** parentData = mObj->registry.get_component_data_by_suid(parentSUID, nullptr);

    if (parentSUID && !parentData) // bad input, parent does not exist in serial domain
        return {};

    CUID parentCUID = parentData ? (*parentData)->cuid : 0;
    CUID compCUID = mObj->registry.create_component(type, name, parentCUID, hintSUID);

    // TODO: DataRegistry API without CUID -> Component Data chasing.
    return Scene::Component(mObj->registry.get_component_data(compCUID, nullptr));
}

void Scene::destroy_component(CUID compID)
{
    mObj->registry.destroy_component(compID);
}

void Scene::reparent(CUID compID, CUID parentID)
{
    mObj->registry.reparent(compID, parentID);
}

void Scene::get_root_component_cuids(Vector<CUID>& roots)
{
    mObj->registry.get_root_components(roots);
}

void Scene::get_root_components(Vector<Component>& roots)
{
    roots.clear();

    Vector<CUID> rootCUIDs;
    mObj->registry.get_root_components(rootCUIDs);

    // kinda slow for pointer chasing CUID > Component Data
    for (CUID rootCUID : rootCUIDs)
    {
        Scene::Component comp(mObj->registry.get_component_data(rootCUID, nullptr));
        LD_ASSERT(comp);
        roots.push_back(comp);
    }
}

Scene::Component Scene::get_component(CUID compID)
{
    return Scene::Component(mObj->registry.get_component_data(compID, nullptr));
}

Scene::Component Scene::get_component_by_suid(SUID compSUID)
{
    return Scene::Component(mObj->registry.get_component_data_by_suid(compSUID, nullptr));
}

ComponentType Scene::Component::type()
{
    return (*mData)->type;
}

CUID Scene::Component::cuid()
{
    return (*mData)->cuid;
}

SUID Scene::Component::suid()
{
    return (*mData)->suid;
}

RUID Scene::Component::ruid()
{
    return sScene->renderSystemCache.get_component_draw_id((*mData)->cuid);
}

const char* Scene::Component::get_name()
{
    return (*mData)->name;
}

AssetID Scene::Component::get_script_asset_id()
{
    return (*mData)->scriptAssetID;
}

void Scene::Component::set_script_asset_id(AssetID assetID)
{
    (*mData)->scriptAssetID = assetID;
}

void Scene::Component::get_children(Vector<Component>& children)
{
    children.clear();

    // kinda slow with ID -> Data pointer chasing.
    for (ComponentBase* child = (*mData)->child; child; child = child->next)
    {
        Scene::Component childC(sScene->registry.get_component_data(child->cuid, nullptr));
        LD_ASSERT(childC);
        children.push_back(childC);
    }
}

Scene::Component Scene::Component::get_parent()
{
    return Scene::Component(sScene->registry.get_component_data((*mData)->cuid, nullptr));
}

bool Scene::Component::get_transform(TransformEx& transform)
{
    // TODO: DataRegistry API to take ComponentBase* directly
    return sScene->registry.get_component_transform((*mData)->cuid, transform);
}

bool Scene::Component::set_transform(const TransformEx& transform)
{
    // TODO: DataRegistry API to take ComponentBase* directly
    return sScene->registry.set_component_transform((*mData)->cuid, transform);
}

bool Scene::Component::get_transform_2d(Transform2D& transform)
{
    // TODO: DataRegistry API to take ComponentBase* directly
    return sScene->registry.get_component_transform_2d((*mData)->cuid, transform);
}

bool Scene::Component::set_transform_2d(const Transform2D& transform)
{
    // TODO: DataRegistry API to take ComponentBase* directly
    return sScene->registry.set_component_transform_2d((*mData)->cuid, transform);
}

bool Scene::Component::get_world_mat4(Mat4& worldMat4)
{
    ComponentBase* base = *mData;

    return sScene->registry.get_component_world_mat4(base->cuid, worldMat4);
}

void Scene::Component::mark_transform_dirty()
{
    ComponentBase* base = *mData;

    sScene->registry.mark_component_transform_dirty(base->cuid);
}

Scene::Component Scene::get_ruid_component(RUID ruid)
{
    CUID compCUID = mObj->renderSystemCache.get_draw_id_component(ruid);

    return Scene::Component(sScene->registry.get_component_data(compCUID, nullptr));
}

Mat4 Scene::get_ruid_transform_mat4(RUID ruid)
{
    LD_PROFILE_SCOPE;

    CUID compID = get_ruid_component(ruid);

    Mat4 worldMat4;
    mObj->registry.get_component_world_mat4(compID, worldMat4);

    return worldMat4;
}

Scene::AudioSource::AudioSource(Component comp)
{
    if (comp && comp.type() == COMPONENT_TYPE_AUDIO_SOURCE)
    {
        mData = comp.data();
        mAudioSource = (AudioSourceComponent*)mData;
    }
}

Scene::AudioSource::AudioSource(AudioSourceComponent* comp)
{
    if (comp && comp->base && comp->base->type == COMPONENT_TYPE_AUDIO_SOURCE)
    {
        mData = (ComponentBase**)comp;
        mAudioSource = comp;
    }
}

Scene::AudioSource::operator bool() const noexcept
{
    return mAudioSource;
}

void Scene::AudioSource::play()
{
    LD_ASSERT_COMPONENT_LOADED;

    sScene->audioSystemCache.start_playback(mAudioSource->playback);
}

void Scene::AudioSource::pause()
{
    LD_ASSERT_COMPONENT_LOADED;

    sScene->audioSystemCache.pause_playback(mAudioSource->playback);
}

void Scene::AudioSource::resume()
{
    LD_ASSERT_COMPONENT_LOADED;

    sScene->audioSystemCache.resume_playback(mAudioSource->playback);
}

bool Scene::AudioSource::set_clip_asset(AssetID clipID)
{
    LD_ASSERT_COMPONENT_LOADED;

    AudioClipAsset clipA(sScene->assetManager.get_asset(clipID).unwrap());
    AudioBuffer buffer = sScene->audioSystemCache.get_or_create_audio_buffer(clipA);

    if (!buffer)
    {
        mAudioSource->clipID = clipID;
        sScene->audioSystemCache.set_playback_buffer(mAudioSource->playback, buffer);
    }

    return false;
}

AssetID Scene::AudioSource::get_clip_asset()
{
    LD_ASSERT_COMPONENT_LOADED;

    return mAudioSource->clipID;
}

float Scene::AudioSource::get_volume_linear()
{
    LD_ASSERT_COMPONENT_LOADED;

    return mAudioSource->volumeLinear;
}

bool Scene::AudioSource::set_volume_linear(float volume)
{
    LD_ASSERT_COMPONENT_LOADED;

    AudioPlayback::Accessor accessor = mAudioSource->playback.access();

    volume = std::clamp(volume, 0.0f, 1.0f);
    mAudioSource->volumeLinear = volume;
    accessor.set_volume_linear(volume);

    return true;
}

float Scene::AudioSource::get_pan()
{
    LD_ASSERT_COMPONENT_LOADED;

    return mAudioSource->pan;
}

bool Scene::AudioSource::set_pan(float pan)
{
    LD_ASSERT_COMPONENT_LOADED;

    AudioPlayback::Accessor accessor = mAudioSource->playback.access();

    pan = std::clamp(pan, 0.0f, 1.0f);
    mAudioSource->pan = pan;
    accessor.set_pan(pan);

    return true;
}

Scene::Camera::Camera(Component comp)
{
    LD_ASSERT_COMPONENT_LOADED;

    if (comp && comp.type() == COMPONENT_TYPE_CAMERA)
    {
        mData = comp.data();
        mCamera = (CameraComponent*)mData;
    }
}

Scene::Camera::Camera(CameraComponent* comp)
{
    LD_ASSERT_COMPONENT_LOADED;

    if (comp && comp->base && comp->base->cuid)
    {
        mData = (ComponentBase**)comp;
        mCamera = comp;
    }
}

Scene::Camera::operator bool() const noexcept
{
    return mCamera && mCamera->camera;
}

bool Scene::Camera::is_main_camera()
{
    LD_ASSERT_COMPONENT_LOADED;

    return mCamera->isMainCamera;
}

bool Scene::Camera::is_perspective()
{
    LD_ASSERT_COMPONENT_LOADED;

    return mCamera->isPerspective;
}

bool Scene::Camera::get_perspective_info(CameraPerspectiveInfo& outInfo)
{
    LD_ASSERT_COMPONENT_LOADED;

    if (mCamera->isPerspective)
    {
        outInfo = mCamera->perspective;
        return true;
    }

    return false;
}

bool Scene::Camera::get_orthographic_info(CameraOrthographicInfo& outInfo)
{
    LD_ASSERT_COMPONENT_LOADED;

    if (!mCamera->isPerspective)
    {
        outInfo = mCamera->orthographic;
        return true;
    }

    return false;
}

void Scene::Camera::set_perspective(const CameraPerspectiveInfo& info)
{
    LD_ASSERT_COMPONENT_LOADED;

    mCamera->isPerspective = true;
    mCamera->perspective = info;
    mCamera->camera.set_perspective(info);
}

void Scene::Camera::set_orthographic(const CameraOrthographicInfo& info)
{
    LD_ASSERT_COMPONENT_LOADED;

    mCamera->isPerspective = false;
    mCamera->orthographic = info;
    mCamera->camera.set_orthographic(info);
}

Scene::Mesh::Mesh(Component comp)
{
    if (comp && comp.type() == COMPONENT_TYPE_MESH)
    {
        mData = comp.data();
        mMesh = (MeshComponent*)mData;
    }
}

Scene::Mesh::Mesh(MeshComponent* comp)
{
    if (comp && comp->base && comp->base->cuid)
    {
        mData = (ComponentBase**)comp;
        mMesh = comp;
    }
}

Scene::Mesh::Mesh::operator bool() const noexcept
{
    return mMesh && mMesh->draw;
}

bool Scene::Mesh::set_mesh_asset(AssetID meshID)
{
    LD_ASSERT_COMPONENT_LOADED;

    MeshData meshData = sScene->renderSystemCache.get_or_create_mesh_data(meshID);

    if (mMesh->draw.set_mesh_asset(meshData))
    {
        mMesh->assetID = meshID;
        return true;
    }

    return false;
}

AssetID Scene::Mesh::get_mesh_asset()
{
    LD_ASSERT_COMPONENT_LOADED;

    return mMesh->assetID;
}

Scene::Sprite2D::Sprite2D(Component comp)
{
    if (comp && comp.type() == COMPONENT_TYPE_SPRITE_2D)
    {
        mData = comp.data();
        mSprite = (Sprite2DComponent*)mData;
    }
}

Scene::Sprite2D::Sprite2D(Sprite2DComponent* comp)
{
    if (comp && comp->base && comp->base->cuid)
    {
        mData = (ComponentBase**)comp;
        mSprite = comp;
    }
}

Scene::Sprite2D::operator bool() const noexcept
{
    return mSprite && mSprite->draw;
}

bool Scene::Sprite2D::set_texture_2d_asset(AssetID textureID)
{
    LD_ASSERT_COMPONENT_LOADED;

    Image2D image = sScene->renderSystemCache.get_or_create_image_2d(textureID);

    if (mSprite->draw.set_image(image))
    {
        mSprite->assetID = textureID;
        return true;
    }

    return false;
}

AssetID Scene::Sprite2D::get_texture_2d_asset()
{
    LD_ASSERT_COMPONENT_LOADED;

    return mSprite->assetID;
}

uint32_t Scene::Sprite2D::get_z_depth()
{
    LD_ASSERT_COMPONENT_LOADED;

    return mSprite->draw.get_z_depth();
}

void Scene::Sprite2D::set_z_depth(uint32_t zDepth)
{
    LD_ASSERT_COMPONENT_LOADED;

    mSprite->draw.set_z_depth(zDepth);
}

Rect Scene::Sprite2D::get_rect()
{
    LD_ASSERT_COMPONENT_LOADED;

    return mSprite->draw.get_rect();
}

void Scene::Sprite2D::set_rect(const Rect& rect)
{
    LD_ASSERT_COMPONENT_LOADED;

    mSprite->draw.set_rect(rect);
}

} // namespace LD