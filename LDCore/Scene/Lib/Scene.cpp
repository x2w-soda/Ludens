#include "LuaScript.h"
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

/// @brief Scene implementation.
struct SceneObj
{
    DataRegistry registry;
    DataRegistry registryBack;
    AssetManager assetManager;
    RServer renderServer;
    LuaState lua;
    std::unordered_map<RUID, CUID> ruidToCuid; /// map draw call to corresponding component
    std::unordered_map<CUID, RUID> cuidToRuid; /// map component to corresponding draw call
    std::unordered_map<AUID, RUID> auidToRuid; /// map asset to GPU resource
    bool hasStartup = false;

    /// @brief Prepare components recursively, creating resources from systems/servers.
    void prepare(ComponentBase* comp);

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
};

static void prepare_mesh_component(SceneObj* scene, CUID compID)
{
    ComponentType componentType;
    MeshComponent* meshC = (MeshComponent*)scene->registry.get_component(compID, componentType);
    LD_ASSERT(meshC && componentType == COMPONENT_TYPE_MESH);

    if (meshC->auid)
    {
        MeshAsset meshA = scene->assetManager.get_mesh_asset(meshC->auid);

        if (!scene->auidToRuid.contains(meshA.get_auid()))
            scene->auidToRuid[meshC->auid] = scene->renderServer.create_mesh(*meshA.data());

        RUID mesh = scene->auidToRuid[meshC->auid];
        RUID drawCall = scene->renderServer.create_mesh_draw_call(mesh);
        scene->ruidToCuid[drawCall] = compID;
        scene->cuidToRuid[compID] = drawCall;
    }
}

/// @brief Component behavior and operations within a Scene.
struct SceneComponent
{
    ComponentType type;
    void (*prepare)(SceneObj* scene, CUID compID);
};

// clang-format off
static SceneComponent sSceneComponents[] = {
    {COMPONENT_TYPE_DATA,       nullptr},
    {COMPONENT_TYPE_TRANSFORM,  nullptr},
    {COMPONENT_TYPE_MESH,       prepare_mesh_component},
    {COMPONENT_TYPE_SPRITE_2D,  nullptr},
};
// clang-format on

static_assert(sizeof(sSceneComponents) / sizeof(*sSceneComponents) == COMPONENT_TYPE_ENUM_COUNT);

void SceneObj::prepare(ComponentBase* comp)
{
    // type-specific preparations
    if (sSceneComponents[comp->type].prepare)
        sSceneComponents[comp->type].prepare(this, comp->id);

    for (ComponentBase* child = comp->child; child; child = child->next)
    {
        prepare(child);
    }
}

void SceneObj::startup_root(CUID root)
{
    const ComponentBase* rootC = registry.get_component_base(root);

    if (!rootC)
        return;

    for (ComponentBase* childC = rootC->child; childC; childC = childC->next)
    {
        startup_root(childC->id);
    }

    // post-order traversal, all child components of root already have their scripts attached
    ComponentScriptSlot* script = registry.get_component_script(rootC->id);
    create_lua_script(script);
    attach_lua_script(script);
}

void SceneObj::cleanup_root(CUID root)
{
    const ComponentBase* rootC = registry.get_component_base(root);

    if (!rootC)
        return;

    for (ComponentBase* childC = rootC->child; childC; childC = childC->next)
    {
        cleanup_root(childC->id);
    }

    // post-order traversal, all child components of root already have their scripts detached
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
    LD_ASSERT(isScriptValid); // TODO: error control flow
    lua.set_table(-3);        // store script instance as ludens.scripts[compID]

    ComponentType type;
    void* comp = registry.get_component(compID, type);

    // create and store table for component type
    LuaScript::create_component_table(registry, lua, compID, type, comp);

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
    LuaScript::destroy_component_table(registry, lua, compID);

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

Scene Scene::create()
{
    SceneObj* obj = heap_new<SceneObj>(MEMORY_USAGE_SCENE);
    obj->registry = DataRegistry::create();
    obj->assetManager = {};
    obj->renderServer = {};

    // lua scripting context
    LuaStateInfo stateI{};
    stateI.openLibs = true;
    obj->lua = LuaState::create(stateI);
    obj->initialize_lua_state(obj->lua);

    return Scene(obj);
}

void Scene::destroy(Scene scene)
{
    SceneObj* obj = scene.unwrap();

    if (obj->registryBack)
        DataRegistry::destroy(obj->registryBack);

    DataRegistry::destroy(obj->registry);
    LuaState::destroy(obj->lua);

    heap_delete<SceneObj>(obj);
}

void Scene::prepare(const ScenePrepareInfo& info)
{
    LD_PROFILE_SCOPE;
    LD_ASSERT(info.assetManager && info.renderServer);

    mObj->assetManager = info.assetManager;
    mObj->renderServer = info.renderServer;

    std::vector<CUID> roots;
    mObj->registry.get_root_components(roots);

    for (CUID rootID : roots)
    {
        ComponentBase* base = mObj->registry.get_component_base(rootID);
        mObj->prepare(base);
    }
}

void Scene::startup()
{
    LD_PROFILE_SCOPE;

    if (mObj->hasStartup)
        return;

    mObj->hasStartup = true;
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

    if (!mObj->hasStartup)
        return;

    mObj->hasStartup = false;
    mObj->lua.get_global("ludens");
    mObj->lua.get_field(-1, "scripts");

    std::vector<CUID> roots;
    mObj->registry.get_root_components(roots);

    for (CUID root : roots)
    {
        mObj->cleanup_root(root);
    }

    mObj->lua.pop(2);
}

void Scene::backup()
{
    LD_PROFILE_SCOPE;

    if (mObj->hasStartup)
        return;

    if (mObj->registryBack)
        DataRegistry::destroy(mObj->registryBack);

    mObj->registryBack = mObj->registry.duplicate();
}

void Scene::swap()
{
    if (mObj->hasStartup)
        return;

    DataRegistry tmp = mObj->registry;
    mObj->registry = mObj->registryBack;
    mObj->registryBack = tmp;
}

void Scene::update(float delta)
{
    LD_PROFILE_SCOPE;

    LuaState L = mObj->lua;
    L.get_global("ludens");
    L.get_field(-1, "scripts");

    for (auto ite = mObj->registry.get_component_scripts(); ite; ++ite)
    {
        auto* script = (ComponentScriptSlot*)ite.data();
        if (!script->isEnabled)
            continue;

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
        L.call(3, 0);

        L.pop(1);
    }

    L.pop(2);
}

CUID Scene::create_component(ComponentType type, const char* name, CUID parent, CUID hint)
{
    return mObj->registry.create_component(type, name, parent, hint);
}

ComponentScriptSlot* Scene::create_component_script_slot(CUID compID, AUID assetID)
{
    return mObj->registry.create_component_script_slot(compID, assetID);
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

void Scene::set_mesh_component_asset(CUID meshCompID, AUID meshAssetID)
{
    if (!mObj->auidToRuid.contains(meshAssetID))
        return;

    RUID mesh = mObj->auidToRuid[meshAssetID];
    if (!mObj->renderServer.mesh_exists(mesh))
        return;

    ComponentType type;
    MeshComponent* meshC = (MeshComponent*)mObj->registry.get_component(meshCompID, type);
    if (!meshC || type != COMPONENT_TYPE_MESH)
        return;

    meshC->auid = meshAssetID;

    auto ite = mObj->cuidToRuid.find(meshCompID);
    if (ite != mObj->cuidToRuid.end())
    {
        RUID oldDrawCall = ite->second;
        mObj->renderServer.destroy_mesh_draw_call(oldDrawCall);
        mObj->ruidToCuid.erase(oldDrawCall);
        mObj->cuidToRuid.erase(meshCompID);
    }

    RUID drawCall = mObj->renderServer.create_mesh_draw_call(mesh);
    mObj->cuidToRuid[meshCompID] = drawCall;
    mObj->ruidToCuid[drawCall] = meshCompID;
}

} // namespace LD