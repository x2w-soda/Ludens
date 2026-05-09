#pragma once

#include <Ludens/Asset/AssetManager.h>
#include <Ludens/DataRegistry/DataRegistryDef.h>
#include <Ludens/Scene/ComponentView.h>
#include <Ludens/Serial/Property.h>

namespace LD {

struct EditorContextObj;

enum EditCommandType
{
    EDIT_COMMAND_TYPE_RENAME_ASSET,
    EDIT_COMMAND_TYPE_RENAME_SCENE,
    EDIT_COMMAND_TYPE_RENAME_COMPONENT,
    EDIT_COMMAND_TYPE_ADD_COMPONENT,
    EDIT_COMMAND_TYPE_SET_COMPONENT_SCRIPT,
    EDIT_COMMAND_TYPE_SET_COMPONENT_ASSET,
    EDIT_COMMAND_TYPE_SET_COMPONENT_TRANSFORM_2D,
    EDIT_COMMAND_TYPE_SET_COMPONENT_PROPS,
    EDIT_COMMAND_TYPE_CLONE_COMPONENT_SUBTREE,
    EDIT_COMMAND_TYPE_DELETE_COMPONENT_SUBTREE,
    EDIT_COMMAND_TYPE_ENUM_COUNT,
};

struct EditCommand
{
    EditCommandType type;
    EditorContextObj* ctx;

    static void redo(EditCommand* cmd);
    static void undo(EditCommand* cmd);
    static EditCommand* create(EditCommandType type, EditorContextObj* ctx);
    static void destroy(EditCommand* cmd);
};

struct RenameAssetCommand : EditCommand
{
    AssetID assetID = 0;
    String oldPath;
    String newPath;

    void configure(AssetID assetID, const String& newPath);
};

struct RenameSceneCommand : EditCommand
{
    SUID sceneID = 0;
    String oldPath;
    String newPath;

    void configure(SUID sceneID, const String& newPath);
};

struct RenameComponentCommand : EditCommand
{
    SUID compSUID = 0;
    String oldName;
    String newName;

    void configure(SUID compSUID, const String& newName);
};

struct AddComponentCommand : EditCommand
{
    SUID parentSUID = 0;
    SUID compSUID = 0;
    ComponentType compType;

    void configure(SUID parentSUID, ComponentType compType);
};

/// @brief Command to set associate a Script with a Component in scene.
struct SetComponentScriptCommand : EditCommand
{
    SUID compSUID = 0;
    AssetID scriptAssetID = 0;
    AssetID prevScriptAssetID = 0;

    void configure(SUID compSUID, AssetID scriptAssetID);
};

struct SetComponentAssetCommand : EditCommand
{
    SUID compSUID = 0;
    AssetID assetID = 0;
    AssetID prevAssetID = 0;
    uint32_t assetSlotIndex = 0;

    void configure(SUID compSUID, AssetID assetID, uint32_t assetSlotIndex);
};

struct SetComponentTransform2DCommand : EditCommand
{
    SUID compSUID;
    Transform2D transform;
    Transform2D prevTransform;
};

struct SetComponentPropsCommand : EditCommand
{
    SUID compSUID;
    Vector<PropertyDelta> delta;
};

struct CloneComponentSubtreeCommand : EditCommand
{
    Vector<int> srcPath;
    CUID dstCUID = 0;

    void configure(SUID compSUID);
};

struct DeleteComponentSubtreeCommand : EditCommand
{
    SUID rootSUID = 0;   // root of subtree to delete
    SUID parentSUID = 0; // parent of subtree
    ComponentSubtreeEntry subtree;

    void configure(SUID compSUID);
};

} // namespace LD
