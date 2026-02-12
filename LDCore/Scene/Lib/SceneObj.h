#pragma once

#include "AudioSystemCache.h"
#include "LuaScript.h"
#include "RenderSystemCache.h"

namespace LD {

enum SceneState
{
    SCENE_STATE_EMPTY = 0,
    SCENE_STATE_LOADED,
    SCENE_STATE_RUNNING,
};

/// @brief Scene implementation.
class SceneObj
{
public:
    DataRegistry registry;
    DataRegistry registryBackup;
    AssetManager assetManager{};
    AudioSystemCache audioSystemCache;
    RenderSystemCache renderSystemCache;
    LuaScript::Context luaContext{};
    CameraComponent* mainCameraC;
    Vec2 screenExtent = {};
    SceneState state = SCENE_STATE_EMPTY;

    void load_registry_from_backup();
    void unload_registry();
    void startup_registry();
    void cleanup_registry();

private:

    bool load_subtree_from_backup(ComponentBase** dstData, ComponentBase** srcData);

    /// @brief Unload components recursively, destroying resources from systems/servers.
    void unload_subtree(ComponentBase** data);

    /// @brief Startup a component subtree recursively, attaching scripts to components
    void startup_subtree(ComponentBase** data);

    /// @brief Cleanup a component subtree recursively, detaching scripts from components
    void cleanup_subtree(ComponentBase** data);
};

extern SceneObj* sScene;

} // namespace LD