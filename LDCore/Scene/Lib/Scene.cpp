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

#include "AudioSystemCache.h"
#include "LuaScript.h"
#include "RenderSystemCache.h"
#include "SceneObj.h"

#include "Component/AudioSourceComponent.h"
#include "Component/CameraComponent.h"
#include "Component/MeshComponent.h"
#include "Component/ScreenUIComponent.h"
#include "Component/Sprite2DComponent.h"

// Scene user's responsibility to check handle before calling methods
#define LD_ASSERT_COMPONENT_LOADED(DATA) LD_ASSERT(DATA && *(DATA) && ((*(DATA))->flags & COMPONENT_FLAG_LOADED_BIT))

namespace LD {

/// @brief Scene Singleton, all scene operations including transition should be done in-place,
///        the SceneObj address should be immutable.
SceneObj* sScene = nullptr;

/// @brief Component behavior and operations within a Scene.
///        - loading a component creates subsystem resources
///        - cloning loads an empty component from some loaded component
///        - startup a component to prepare it for runtime
///        - cleanup a component to release runtime resources
///        - unloading a component destroys subsystem resources
struct SceneComponent
{
    ComponentType type;
    bool (*clone)(SceneObj* scene, ComponentBase** dstData, ComponentBase** srcData);
    void (*unload)(SceneObj* scene, ComponentBase** data);
    void (*startup)(SceneObj* scene, ComponentBase** data);
    void (*cleanup)(SceneObj* scene, ComponentBase** data);
};

// clang-format off
static SceneComponent sSceneComponents[] = {
    {COMPONENT_TYPE_DATA,         nullptr,                       nullptr,                        nullptr,                      nullptr},
    {COMPONENT_TYPE_AUDIO_SOURCE, &clone_audio_source_component, &unload_audio_source_component, nullptr,                      &cleanup_audio_source_component},
    {COMPONENT_TYPE_TRANSFORM,    nullptr,                       nullptr,                        nullptr,                      nullptr},
    {COMPONENT_TYPE_CAMERA,       &clone_camera_component,       &unload_camera_component,       &startup_camera_component,    &cleanup_camera_component},
    {COMPONENT_TYPE_MESH,         &clone_mesh_component,         &unload_mesh_component,         nullptr,                      nullptr},
    {COMPONENT_TYPE_SPRITE_2D,    &clone_sprite_2d_component,    &unload_sprite_2d_component,    nullptr,                      nullptr},
    {COMPONENT_TYPE_SCREEN_UI,    &clone_screen_ui_component,    &unload_screen_ui_component,    &startup_screen_ui_component, &cleanup_screen_ui_component},
};
// clang-format on

static_assert(sizeof(sSceneComponents) / sizeof(*sSceneComponents) == COMPONENT_TYPE_ENUM_COUNT);

//
// IMPLEMENTATION
//

void SceneObj::load_registry_from_backup()
{
    LD_PROFILE_SCOPE;

    LD_ASSERT(registry && registryBackup);

    Vector<ComponentBase**> dstRoots;
    Vector<ComponentBase**> srcRoots;
    registry.get_root_component_data(dstRoots);
    registryBackup.get_root_component_data(srcRoots);
    LD_ASSERT(dstRoots.size() == srcRoots.size());

    for (size_t i = 0; i < dstRoots.size(); i++)
    {
        bool ok = load_subtree_from_backup(dstRoots[i], srcRoots[i]);
        LD_ASSERT(ok);
    }
}

void SceneObj::unload_registry()
{
    LD_PROFILE_SCOPE;

    Vector<ComponentBase**> roots;
    registry.get_root_component_data(roots);

    for (ComponentBase** rootData : roots)
    {
        unload_subtree(rootData);
    }
}

void SceneObj::startup_registry()
{
    LD_PROFILE_SCOPE;

    Vector<ComponentBase**> roots;
    registry.get_root_component_data(roots);

    for (ComponentBase** rootData : roots)
    {
        startup_subtree(rootData);
    }
}

void SceneObj::cleanup_registry()
{
    LD_PROFILE_SCOPE;

    Vector<ComponentBase**> roots;
    registry.get_root_component_data(roots);

    for (ComponentBase** rootData : roots)
    {
        cleanup_subtree(rootData);
    }

    mainCameraC = nullptr;
}

// This is basically the editor loading a copy of component subtree from backup data
bool SceneObj::load_subtree_from_backup(ComponentBase** dstData, ComponentBase** srcData)
{
    LD_PROFILE_SCOPE;

    LD_ASSERT(dstData && srcData);

    ComponentBase* dstBase = *dstData;
    ComponentBase* srcBase = *srcData;

    // sanity checks
    LD_ASSERT(srcBase->type == dstBase->type);
    LD_ASSERT(srcBase->suid == dstBase->suid);
    LD_ASSERT((srcBase->flags & COMPONENT_FLAG_LOADED_BIT) && ~(dstBase->flags & COMPONENT_FLAG_LOADED_BIT));
    LD_ASSERT((std::string(dstBase->name) == std::string(srcBase->name)));

    bool ok = sSceneComponents[(int)dstBase->type].clone(sScene, dstData, srcData);
    LD_ASSERT(ok);
    if (!ok)
        return false;

    ComponentBase* dstChild = dstBase->child;
    ComponentBase* srcChild = srcBase->child;

    while (dstChild)
    {
        LD_ASSERT(dstChild && srcChild);
        ComponentBase** dstChildData = registry.get_component_data(dstChild->cuid, nullptr);
        ComponentBase** srcChildData = registryBackup.get_component_data(srcChild->cuid, nullptr);

        if (!load_subtree_from_backup(dstChildData, srcChildData))
            return false;

        dstChild = dstChild->next;
        srcChild = srcChild->next;
    }

    return true;
}

void SceneObj::unload_subtree(ComponentBase** data)
{
    LD_PROFILE_SCOPE;

    LD_ASSERT(data);
    ComponentBase* base = (*data);
    LD_ASSERT(base->flags & COMPONENT_FLAG_LOADED_BIT);

    if (sSceneComponents[(int)base->type].unload)
        sSceneComponents[(int)base->type].unload(this, data);

    // sanity check
    LD_ASSERT((base->flags & COMPONENT_FLAG_LOADED_BIT) == 0);

    for (ComponentBase* child = base->child; child; child = child->next)
    {
        ComponentBase** childData = registry.get_component_data(child->cuid, nullptr);
        unload_subtree(childData);
    }
}

void SceneObj::startup_subtree(ComponentBase** rootData)
{
    LD_ASSERT(rootData);
    ComponentBase* rootBase = *rootData;
    LD_ASSERT(rootBase->flags & COMPONENT_FLAG_LOADED_BIT);

    for (ComponentBase* childBase = rootBase->child; childBase; childBase = childBase->next)
    {
        ComponentBase** childData = registry.get_component_data(childBase->cuid, nullptr);
        startup_subtree(childData);
    }

    // post-order traversal, all child components of root already have their scripts attached
    if (sSceneComponents[(int)rootBase->type].startup)
        sSceneComponents[(int)rootBase->type].startup(this, rootData);

    if (rootBase->scriptAssetID)
    {
        const CUID rootCUID = rootBase->cuid;
        luaContext.create_component_table(rootCUID);
        bool success = luaContext.create_lua_script(rootCUID, rootBase->scriptAssetID); // TODO: abort startup at the first failure of creating lua script instance.
        LD_ASSERT(success);
        luaContext.attach_lua_script(rootCUID);
    }
}

void SceneObj::cleanup_subtree(ComponentBase** rootData)
{
    LD_ASSERT(rootData);
    ComponentBase* rootBase = *rootData;

    for (ComponentBase* childBase = rootBase->child; childBase; childBase = childBase->next)
    {
        ComponentBase** childData = registry.get_component_data(childBase->cuid, nullptr);
        cleanup_subtree(childData);
    }

    // post-order traversal, all child components of root already have their scripts detached
    if (sSceneComponents[(int)rootBase->type].cleanup)
        sSceneComponents[(int)rootBase->type].cleanup(this, rootData);

    const CUID rootCUID = rootBase->cuid;
    luaContext.detach_lua_script(rootCUID);
    luaContext.destroy_lua_script(rootCUID);
    luaContext.destroy_component_table(rootCUID);
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
    sScene->renderSystemCache.create(sceneI.renderSystem, sceneI.assetManager);
    sScene->audioSystemCache.create(sceneI.audioSystem, sceneI.assetManager);
    sScene->luaContext.create(Scene(sScene), sceneI.assetManager);
    sScene->screenExtent = sceneI.extent;

    ScreenUIInfo info{};
    info.extent = sScene->screenExtent;
    info.fontAtlas = sceneI.fontAtlas;
    info.fontAtlasImage = sceneI.fontAtlasImage;
    info.theme = sceneI.uiTheme;
    LD_ASSERT(info.fontAtlas);
    LD_ASSERT(info.fontAtlasImage);
    LD_ASSERT(info.theme);
    sScene->screenUI = LD::ScreenUI::create(info);
    sScene->screenUIBackup = {}; // TODO:

    return Scene(sScene);
}

void Scene::destroy(Scene scene)
{
    LD_PROFILE_SCOPE;

    LD_ASSERT(sScene && sScene == scene.unwrap());

    // destroy all components
    scene.reset();

    LD::ScreenUI::destroy(sScene->screenUI);
    sScene->luaContext.destroy();
    sScene->audioSystemCache.destroy();
    sScene->renderSystemCache.destroy();
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
    LD_ASSERT(!mObj->registryBackup);

    if (mObj->registry)
    {
        DataRegistry::destroy(mObj->registry);
        mObj->registry = {};
    }

    mObj->mainCameraC = nullptr;
    mObj->registry = DataRegistry::create();
}

void Scene::load(const std::function<bool(Scene)>& loader)
{
    LD_PROFILE_SCOPE;

    if (mObj->state == SCENE_STATE_LOADED)
    {
        LD_UNREACHABLE;
        return;
    }

    bool success = loader(*this);
    if (!success)
    {
        // TODO: unwind
        LD_UNREACHABLE;
        return;
    }

    /*
    if (!validate())
    {
        // TODO: unwind
        return;
    }
    */

    // force initial UI layout
    mObj->screenUI.update(0.0f);

    mObj->state = SCENE_STATE_LOADED;
}

void Scene::unload()
{
    LD_PROFILE_SCOPE;

    if (mObj->state == SCENE_STATE_EMPTY)
        return;

    mObj->unload_registry();

    mObj->state = SCENE_STATE_EMPTY;
}

void Scene::backup()
{
    LD_PROFILE_SCOPE;

    LD_ASSERT(mObj->state == SCENE_STATE_LOADED);
    LD_ASSERT(!mObj->registryBackup);
    LD_ASSERT(!mObj->screenUIBackup);

    mObj->screenUIBackup = mObj->screenUI;
    mObj->screenUI;

    // create and load a copy for play-in-editor session
    DataRegistry pie = mObj->registry.duplicate();
    mObj->registryBackup = mObj->registry;
    mObj->registry = pie;
    mObj->load_registry_from_backup();
}

void Scene::startup()
{
    LD_PROFILE_SCOPE;

    if (mObj->state == SCENE_STATE_RUNNING)
        return;

    mObj->state = SCENE_STATE_RUNNING;

    mObj->startup_registry();
}

void Scene::cleanup()
{
    LD_PROFILE_SCOPE;

    if (mObj->state != SCENE_STATE_RUNNING)
        return;

    mObj->state = SCENE_STATE_LOADED;

    mObj->cleanup_registry();

    // after play-in-editor session, restore the backup.
    if (mObj->registryBackup)
    {
        mObj->unload_registry();
        DataRegistry::destroy(mObj->registry);
        mObj->registry = mObj->registryBackup;
        mObj->registryBackup = {};
    }
}

void Scene::update(const Vec2& screenExtent, float delta)
{
    LD_PROFILE_SCOPE;
    LD_ASSERT(screenExtent.x > 0.0f && screenExtent.y > 0.0f);

    if (mObj->screenExtent != screenExtent)
    {
        mObj->screenExtent = screenExtent;
        mObj->screenUI.resize(screenExtent);
    }

    // update all lua script instances
    mObj->luaContext.update(delta);

    // update screen space UI
    mObj->screenUI.update(delta);

    if (mObj->mainCameraC)
    {
        const CameraComponent* cameraC = mObj->mainCameraC;
        const ComponentBase* base = cameraC->base;
        LD::Camera mainCamera = cameraC->camera;
        LD_ASSERT(base && mainCamera);

        Mat4 worldMat4;
        mObj->registry.get_component_world_mat4(base->cuid, worldMat4);
        Vec3 forward = worldMat4.as_mat3() * Vec3(0.0f, 0.0f, 1.0f);

        mainCamera.set_aspect_ratio(screenExtent.x / screenExtent.y);
        mainCamera.set_pos(cameraC->transform.position);
        mainCamera.set_target(cameraC->transform.position + forward);
    }

    // any heap allocations for audio is done on main thread.
    mObj->audioSystemCache.update();
}

void Scene::render_screen_ui(ScreenRenderComponent renderer)
{
    LD_PROFILE_SCOPE;

    mObj->screenUI.render(renderer);
}

void Scene::input_screen_ui(const WindowEvent* event)
{
    LD_PROFILE_SCOPE;

    mObj->screenUI.input(event);
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

void Scene::get_root_components(Vector<Component>& roots)
{
    roots.clear();

    Vector<ComponentBase**> rootData;
    mObj->registry.get_root_component_data(rootData);

    // kinda slow for pointer chasing CUID > Component Data
    for (ComponentBase** root : rootData)
    {
        Scene::Component comp(root);
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
    ComponentBase* parentBase = (*mData)->parent;
    if (!parentBase)
        return {};

    return Scene::Component(sScene->registry.get_component_data(parentBase->cuid, nullptr));
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

Scene::Component Scene::get_ruid_component(RUID ruid)
{
    CUID compCUID = mObj->renderSystemCache.get_draw_id_component(ruid);

    return Scene::Component(sScene->registry.get_component_data(compCUID, nullptr));
}

bool Scene::get_ruid_world_mat4(RUID ruid, Mat4& worldMat4)
{
    LD_PROFILE_SCOPE;

    Scene::Component comp = get_ruid_component(ruid);
    if (!comp || !mObj->registry.get_component_world_mat4(comp.cuid(), worldMat4))
        return false;

    return true;
}

} // namespace LD