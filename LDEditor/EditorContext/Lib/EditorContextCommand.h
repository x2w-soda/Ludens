#pragma once

#include <Ludens/Asset/AssetManager.h>
#include <Ludens/DataRegistry/DataComponent.h>
#include <Ludens/Scene/SceneSchema.h>
#include <LudensEditor/EditorContext/EditStack.h>

namespace LD {

class AddComponentCommand : public EditCommand
{
public:
    AddComponentCommand(Scene scene, SUID parentSUID, ComponentType compType);

    virtual void redo() override;
    virtual void undo() override;

private:
    Scene mScene;
    SUID mParentSUID = 0;
    SUID mCompSUID = 0;
    ComponentType mCompType;
};

/// @brief Command to set associate a Script with a Component in scene.
class AddComponentScriptCommand : public EditCommand
{
public:
    AddComponentScriptCommand(Scene scene, SUID compSUID, AssetID scriptAssetID);

    virtual void redo() override;
    virtual void undo() override;

private:
    Scene mScene;
    SUID mCompSUID = 0;
    AssetID mScriptAssetID = 0;
    AssetID mPrevScriptAssetID = 0;
};

class SetComponentAssetCommand : public EditCommand
{
public:
    SetComponentAssetCommand(Scene scene, SUID compSUID, AssetID assetID);

    virtual void redo() override;
    virtual void undo() override;

private:
    void set_component_asset(SUID compID, AssetID assetID);

private:
    Scene mScene;
    SUID mCompSUID = 0;
    AssetID mAssetID = 0;
    AssetID mPrevAssetID = 0;
};

} // namespace LD