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
    AssetManager assetManager;
    RServer renderServer;
    LuaState lua;
    std::unordered_map<RUID, Transform*> ruidTransforms; /// map RUID to its corresponding transform
    std::unordered_map<RUID, DUID> ruidToComponent;      /// map RUID to its corresponding component in the data registry

    /// @brief Prepare components recursively, loading scripts and assets.
    void prepare(ComponentBase* comp);

    /// @brief Startup a component subtree recursively, attaching scripts to components
    void startup_root(DUID compID);

    /// @brief Cleanup a component subtree recursively, detaching scripts from components
    void cleanup_root(DUID compID);

    /// @brief Create lua script associated with a component.
    void create_lua_script(ComponentScriptSlot* scriptSlot);

    /// @brief Destroy lua script associated with a component
    void destroy_lua_script(DUID compID);

    /// @brief Attach lua script to a data component.
    void attach_lua_script(DUID compID);

    /// @brief Detach lua script from a data component.
    void detach_lua_script(DUID compID);

    /// @brief Initialize a lua state for scripting
    void initialize_lua_state(LuaState L);
};

static void prepare_mesh_component(SceneObj* scene, DUID compID)
{
    ComponentType componentType;
    MeshComponent* meshC = (MeshComponent*)scene->registry.get_component(compID, componentType);
    LD_ASSERT(meshC && componentType == COMPONENT_TYPE_MESH);

    if (meshC->auid)
    {
        MeshAsset meshA = scene->assetManager.get_mesh_asset(meshC->auid);
        meshC->ruid = scene->renderServer.create_mesh(*meshA.data());
        scene->ruidTransforms[meshC->ruid] = scene->registry.get_component_transform(compID);
        scene->ruidToComponent[meshC->ruid] = compID;
    }
}

/// @brief Component behavior and operations within a Scene.
struct SceneComponent
{
    ComponentType type;
    void (*prepare)(SceneObj* scene, DUID compID);
};

// clang-format off
static SceneComponent sSceneComponents[] = {
    {COMPONENT_TYPE_DATA,       nullptr},
    {COMPONENT_TYPE_TRANSFORM,  nullptr},
    {COMPONENT_TYPE_MESH,       prepare_mesh_component},
    {COMPONENT_TYPE_TEXTURE_2D, nullptr},
};
// clang-format on

static_assert(sizeof(sSceneComponents) / sizeof(*sSceneComponents) == COMPONENT_TYPE_ENUM_COUNT);

void SceneObj::prepare(ComponentBase* comp)
{
    ComponentScriptSlot* scriptSlot = registry.get_component_script(comp->id);

    // instantiate script
    if (scriptSlot)
    {
        create_lua_script(scriptSlot);
    }

    // type-specific preparations
    if (sSceneComponents[comp->type].prepare)
        sSceneComponents[comp->type].prepare(this, comp->id);

    for (ComponentBase* child = comp->child; child; child = child->next)
    {
        prepare(child);
    }
}

void SceneObj::startup_root(DUID root)
{
    const ComponentBase* rootC = registry.get_component_base(root);

    if (!rootC)
        return;

    for (ComponentBase* childC = rootC->child; childC; childC = childC->next)
    {
        startup_root(childC->id);
    }

    // post-order traversal, all child components of root already have their scripts attached
    attach_lua_script(rootC->id);
}

void SceneObj::cleanup_root(DUID root)
{
    const ComponentBase* rootC = registry.get_component_base(root);

    if (!rootC)
        return;

    for (ComponentBase* childC = rootC->child; childC; childC = childC->next)
    {
        cleanup_root(childC->id);
    }

    // post-order traversal, all child components of root already have their scripts detached
    detach_lua_script(rootC->id);
}

void SceneObj::create_lua_script(ComponentScriptSlot* scriptSlot)
{
    LD_ASSERT(scriptSlot);
    LD_ASSERT(lua.empty());

    DUID compID = scriptSlot->componentID;
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
    LD_ASSERT(lua.size() == 4);
    lua.set_table(-3); // store script instance as ludens.scripts[compID]

    ComponentType type;
    void* comp = registry.get_component(compID, type);

    // create and store table for component type
    LuaScript::create_component_table(lua, compID, type, comp);

    lua.resize(0);
}

void SceneObj::destroy_lua_script(DUID compID)
{
    int oldSize = lua.size();
    lua.get_global("ludens");
    lua.get_field(-1, "scripts");

    // destroy component lua table representation
    LuaScript::destroy_component_table(lua, compID);

    lua.push_number((double)compID);
    lua.push_nil();
    lua.set_table(-3);

    lua.resize(oldSize);
}

// Caller should prepare ludens.scripts table on top of stack
void SceneObj::attach_lua_script(DUID rootID)
{
    LD_ASSERT(lua.get_type(-1) == LUA_TYPE_TABLE);

    ComponentScriptSlot* script = registry.get_component_script(rootID);
    if (!script)
        return;

    int oldSize = lua.size();

    // call 'attach' lua method on script
    lua.push_number((double)script->componentID);
    lua.get_table(-2);
    LD_ASSERT(lua.get_type(-1) == LUA_TYPE_TABLE); // script instance

    lua.get_field(-1, "attach");
    LD_ASSERT(lua.get_type(-1) == LUA_TYPE_FN); // script attach method

    // arg1 is script instance
    lua.push_number((double)script->componentID);
    lua.get_table(-4);

    // arg2 is the component
    lua.get_field(-3, "_comp");
    LD_ASSERT(lua.get_type(-1) == LUA_TYPE_TABLE);

    lua.call(2, 0);

    lua.resize(oldSize);
}

// Caller should prepare ludens.scripts table on top of stack
void SceneObj::detach_lua_script(DUID rootID)
{
    LD_ASSERT(lua.get_type(-1) == LUA_TYPE_TABLE);

    ComponentScriptSlot* script = registry.get_component_script(rootID);
    if (!script)
        return;

    // call 'detach' lua method on script
    lua.push_number((double)script->componentID);
    lua.get_table(-2);
    lua.get_field(-1, "detach");

    // arg1 is script instance
    lua.push_number((double)script->componentID);
    lua.get_table(-3);

    lua.call(1, 0);
    lua.pop(1);
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

    std::vector<DUID> roots;
    mObj->registry.get_root_components(roots);

    for (DUID rootID : roots)
    {
        ComponentBase* base = mObj->registry.get_component_base(rootID);
        mObj->prepare(base);
    }
}

void Scene::startup()
{
    LD_PROFILE_SCOPE;

    mObj->lua.clear();
    mObj->lua.get_global("ludens");
    mObj->lua.get_field(-1, "scripts");

    std::vector<DUID> roots;
    mObj->registry.get_root_components(roots);

    for (DUID root : roots)
    {
        mObj->startup_root(root);
    }

    mObj->lua.pop(2);
}

void Scene::cleanup()
{
    LD_PROFILE_SCOPE;

    mObj->lua.get_global("ludens");
    mObj->lua.get_field(-1, "scripts");

    std::vector<DUID> roots;
    mObj->registry.get_root_components(roots);

    for (DUID root : roots)
    {
        mObj->cleanup_root(root);
    }

    mObj->lua.pop(2);
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

DUID Scene::create_component(ComponentType type, const char* name, DUID parent, DUID hint)
{
    return mObj->registry.create_component(type, name, parent, hint);
}

ComponentScriptSlot* Scene::create_component_script_slot(DUID compID, AUID assetID)
{
    return mObj->registry.create_component_script_slot(compID, assetID);
}

void Scene::destroy_component(DUID compID)
{
    mObj->registry.destroy_component(compID);
}

void Scene::reparent(DUID compID, DUID parentID)
{
    mObj->registry.reparent(compID, parentID);
}

void Scene::get_root_components(std::vector<DUID>& roots)
{
    mObj->registry.get_root_components(roots);
}

ComponentBase* Scene::get_component_base(DUID compID)
{
    return mObj->registry.get_component_base(compID);
}

void* Scene::get_component(DUID compID, ComponentType& type)
{
    return mObj->registry.get_component(compID, type);
}

RUID Scene::get_component_ruid(DUID compID)
{
    return mObj->registry.get_component_ruid(compID);
}

Transform* Scene::get_component_transform(DUID compID)
{
    // pointer stability: valid until component is destroyed
    return mObj->registry.get_component_transform(compID);
}

DUID Scene::get_ruid_component(RUID ruid)
{
    auto ite = mObj->ruidToComponent.find(ruid);

    if (ite == mObj->ruidToComponent.end())
        return 0;

    return ite->second;
}

Mat4 Scene::get_ruid_transform(RUID ruid)
{
    // TODO: optimize, this is terrible and lazy
    Transform* transform = mObj->ruidTransforms[ruid];
    const Vec3& axisR = transform->rotation;
    Mat4 R = Mat4::rotate(LD_TO_RADIANS(axisR.x), Vec3(1.0f, 0.0f, 0.0f)) * Mat4::rotate(LD_TO_RADIANS(axisR.y), Vec3(0.0f, 1.0f, 0.0f)) * Mat4::rotate(LD_TO_RADIANS(axisR.z), Vec3(0.0f, 0.0f, 1.0f));
    return Mat4::translate(transform->position) * R * Mat4::scale(transform->scale);
}


} // namespace LD