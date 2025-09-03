#pragma once

#include <Ludens/RenderServer/RServer.h>
#include <Ludens/Scene/Scene.h>
#include <LudensEditor/EditorContext/EditorSettings.h>
#include <LudensEditor/EditorContext/EditorContextEvent.h>

namespace LD {

struct ComponentBase;
struct Transform;

struct EditorContextInfo
{
    RServer renderServer; /// render server handle
};

/// @brief Shared context among editor windows. Keeps track of
///        the Scene being edited. All edits to the current Scene
///        goes through this class.
struct EditorContext : Handle<struct EditorContextObj>
{
    /// @brief Create the editor context.
    static EditorContext create(const EditorContextInfo& info);

    /// @brief Destroy the editor context.
    static void destroy(EditorContext ctx);

    /// @brief Callback to inform the render server the transforms of RUIDs
    static Mat4 render_server_transform_callback(RUID ruid, void* user);

    /// @brief Get editor global settings
    EditorSettings get_settings();

    /// @brief Add an observer of the editor context
    void add_observer(EditorContextEventFn fn, void* user);

    /// @brief Editor context frame update, if the scene is playing, this calls the scene update.
    /// @param delta Delta time in seconds.
    void update(float delta);

    ///@brief Load a Project to edit
    ///@warning Experimental
    ///@note Triggers EDITOR_CONTEXT_EVENT_PROJECT_LOAD for observers
    void load_project(const std::filesystem::path& toml);

    /// @brief Load a Scene from the current Project
    /// @warning Experimental
    /// @note Triggers EDITOR_CONTEXT_EVENT_SCENE_LOAD for observers.
    void load_project_scene(const std::filesystem::path& toml);

    /// @brief Begin playing scene in the editor.
    void play_scene();

    /// @brief Stop playing scene in the editor.
    void stop_scene();

    /// @brief Whether or not the scene simulation is playing in editor.
    bool is_playing();

    /// @brief Get root data components in scene
    void get_scene_roots(std::vector<DUID>& roots);

    /// @brief Get component base members.
    const ComponentBase* get_component_base(DUID comp);

    /// @brief Get the C string name of a component
    const char* get_component_name(DUID comp);

    /// @brief Assign a component in scene to be selected.
    /// @note Triggers EDITOR_CONTEXT_EVENT_COMPONENT_SELECTION for observers.
    void set_selected_component(DUID comp);

    /// @brief Get the currently selected component in scene
    DUID get_selected_component();

    /// @brief Get component in current Scene
    void* get_component(DUID compID, ComponentType& type);

    /// @brief Get the data component associated with some RUID in scene
    DUID get_ruid_component(RUID ruid);

    /// @brief get the RUID associated with the selected object in scene
    RUID get_selected_component_ruid();

    /// @brief get the Transform associated with the selected object in scene
    /// @return address of object transform, or nullptr
    /// @warning pointer is invalidated when the component is destroyed.
    Transform* get_selected_component_transform();
};

} // namespace LD