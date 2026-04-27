#pragma once

#include <Ludens/DSA/HashMap.h>
#include <Ludens/DSA/Observer.h>
#include <Ludens/Project/ProjectContext.h>
#include <Ludens/Project/ProjectLoadAsync.h>
#include <LudensBuilder/AssetBuilder/AssetBuilder.h>
#include <LudensBuilder/AssetBuilder/AssetImporter.h>
#include <LudensBuilder/DocumentBuilder/DocumentRegistry.h>
#include <LudensEditor/EditorContext/EditStack.h>
#include <LudensEditor/EditorContext/EditorEventQueue.h>

namespace LD {

/// @brief Editor context implementation. Keeps track of the active project
///        and active scene states.
struct EditorContextObj
{
    RenderSystem renderSystem;                            /// render server handle
    AudioSystem audioSystem;                              /// audio server handle
    Image2D iconAtlas;                                    /// editor icon atlas handle
    ProjectContext projectCtx = {};                       /// project level states
    ProjectLoadAsync projectLoadAsync = {};               /// asynchronous project loading context
    AssetImporter assetImporter = {};                     /// asset batch importer
    AssetBuilder assetBuilder = {};                       /// build assets from within the editor
    Scene scene = {};                                     /// current scene under edit
    EditorSettings settings;                              /// editor global settings
    EditorEventQueue eventQueue;                          /// editor events waiting to be processed
    EditStack editStack;                                  /// undo/redo stack of EditCommands
    UIFontRegistry fontRegistry;                          /// all fonts used by editor are registered here
    UIFont fontDefault;                                   /// editor default regular font
    UIFont fontMono;                                      /// editor default monospace font
    DocumentRegistry docRegistry;                         /// all documents to be displayed in the editor
    FS::Path iconAtlasPath;                               /// path to editor icon atlas source file
    FS::Path sceneSchemaAbsPath;                          /// path to current scene under edit
    SUID activeSceneID = 0;                               /// current scene ID
    HashMap<KeyValue, EditorEventType> keyBinds;          /// key shortcuts to generate events
    HashMap<FS::Path, EditorProjectEntry> projectEntries; /// projects discovered by editor context
    ObserverList<const EditorEvent*> observers;           /// all observers of EditorContext
    CUID selectedComponentCUID = 0;                       /// component selection state
    CUID prevSelectedComponentCUID = 0;                   /// previous component selection state
    bool isPlaying = false;                               /// whether Scene simulation is in progress
    int lastSavedEditIndex = 0;

    void emit_event(EditorEventType type);
    void notify_observers(const EditorEvent* event);
    void load_project_scene(SUID sceneID);
    void save_scene_schema();
    void save_asset_schema();
    void save_project_schema();
    void begin_project_load_async(const FS::Path& projectSchemaPath);
    void update_project_load_async();
    void update_asset_import_async();
    void add_project_entry(const ProjectScanResult& scanResult);
    bool prepare_document(const char* uriPath);
};

} // namespace LD