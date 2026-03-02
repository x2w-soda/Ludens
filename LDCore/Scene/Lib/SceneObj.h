#pragma once

#include <Ludens/UI/UIContext.h>

#include "AudioSystemCache.h"
#include "LuaScript.h"
#include "RenderSystemCache.h"
#include "ScreenUI.h"

namespace LD {

enum SceneState
{
    SCENE_STATE_EMPTY = 0,
    SCENE_STATE_LOADED,
    SCENE_STATE_RUNNING,
};

struct SceneContextInfo
{
    AssetManager assetManager;
    FontAtlas fontAtlas;
    RImage fontAtlasImage;
    UITheme uiTheme;
};

struct SceneContext
{
    LuaScript::Context lua;
    ScreenUI screenUI;
    DataRegistry registry;
    AssetManager assetManager;

    SceneContext() = delete;
    SceneContext(const SceneContextInfo& info);
    SceneContext(const SceneContext&) = delete;
    ~SceneContext();

    SceneContext& operator=(const SceneContext&) = delete;

    void update(const Vec2& screenExtent, float delta);

    bool startup_registry();
    void cleanup_registry();
    void unload_registry();

    /// @brief Startup a component subtree recursively, attaching scripts to components
    bool startup_subtree(ComponentBase** data, Vector<ComponentBase**>& startupOrder, std::string& err);

    bool startup_component(ComponentBase** data, std::string& err);

    /// @brief Cleanup a component subtree recursively, detaching scripts from components
    void cleanup_subtree(ComponentBase** data);

    bool cleanup_component(ComponentBase** data, std::string& err);

    /// @brief Unload components recursively, destroying resources from systems/servers.
    void unload_subtree(ComponentBase** data);
};

/// @brief Scene implementation.
class SceneObj
{
public:
    SceneContext* active;
    SceneContext* backup;
    SceneContextInfo contextInfo{};
    AssetManager assetManager{};
    AudioSystemCache audioSystemCache;
    RenderSystemCache renderSystemCache;
    SceneState state = SCENE_STATE_EMPTY;
    Vec2 extent{};

    bool load_registry_from_backup();

private:
    bool load_subtree_from_backup(ComponentBase** dstData, ComponentBase** srcData, std::string& err);
};

extern SceneObj* sScene;

} // namespace LD