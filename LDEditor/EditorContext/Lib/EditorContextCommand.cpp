#include "EditorContextCommand.h"
#include <Ludens/Header/Assert.h>

namespace LD {

AddComponentScriptCommand::AddComponentScriptCommand(SceneSchema sceneSchema, Scene scene, CUID compID, AUID scriptAssetID)
    : mSchema(sceneSchema), mScene(scene), mCompID(compID), mScriptAssetID(scriptAssetID)
{
    LD_ASSERT(mSchema && mScene && mCompID && mScriptAssetID);

    mPrevScriptAssetID = mSchema.get_component_script(mCompID);
}

void AddComponentScriptCommand::redo()
{
    // update Schema
    mSchema.set_component_script(mCompID, mScriptAssetID);

    // update Scene
    mScene.create_component_script_slot(mCompID, mScriptAssetID);
}

void AddComponentScriptCommand::undo()
{
    // update Schema
    mSchema.set_component_script(mCompID, mPrevScriptAssetID);

    // update Scene
    ComponentScriptSlot* slot = mScene.get_component_script_slot(mCompID);
    LD_ASSERT(slot);

    slot->assetID = mPrevScriptAssetID;
}

} // namespace LD