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
struct SceneObj
{
    DataRegistry registry;
    DataRegistry registryBack;
    AssetManager assetManager{};
    AudioSystemCache audioSystemCache;
    RenderSystemCache renderSystemCache;
    LuaScript::Context luaContext{};
    CameraComponent* mainCameraC;
    Vec2 screenExtent = {};
    SceneState state = SCENE_STATE_EMPTY;

    /// @brief Unload components recursively, destroying resources from systems/servers.
    void unload_subtree(ComponentBase** data);

    /// @brief Startup a component subtree recursively, attaching scripts to components
    void startup_subtree(CUID compID);

    /// @brief Cleanup a component subtree recursively, detaching scripts from components
    void cleanup_subtree(CUID compID);
};

extern SceneObj* sScene;

} // namespace LD