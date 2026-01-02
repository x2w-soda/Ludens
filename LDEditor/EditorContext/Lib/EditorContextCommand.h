#pragma once

#include <Ludens/Asset/AssetManager.h>
#include <Ludens/DataRegistry/DataComponent.h>
#include <Ludens/Scene/SceneSchema.h>
#include <LudensEditor/EditorContext/EditStack.h>

namespace LD {

/// @brief Command to set associate a Script with a Component in scene.
class AddComponentScriptCommand : public EditCommand
{
public:
    AddComponentScriptCommand(Scene scene, CUID compID, AUID scriptAssetID);

    virtual void redo() override;

    virtual void undo() override;

private:
    Scene mScene;
    CUID mCompID;
    AUID mScriptAssetID;
    AUID mPrevScriptAssetID;
};

class SetComponentAssetCommand : public EditCommand
{
public:
    SetComponentAssetCommand(Scene scene, CUID compID, AUID assetID);
    
    virtual void redo() override;
    virtual void undo() override;

private:
    void set_component_asset(CUID compID, AUID assetID);

private:
    Scene mScene;
    CUID mCompID;
    AUID mAssetID;
    AUID mPrevAssetID;
};

} // namespace LD