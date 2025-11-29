#include "EditorContextCommand.h"
#include <Ludens/Header/Assert.h>

namespace LD {

AddComponentScriptCommand::AddComponentScriptCommand(Scene scene, CUID compID, AUID scriptAssetID)
    : mScene(scene), mCompID(compID), mScriptAssetID(scriptAssetID), mPrevScriptAssetID(0)
{
    LD_ASSERT(mScene && mCompID && mScriptAssetID);

    ComponentScriptSlot* scriptSlot = mScene.get_component_script_slot(mCompID);
    if (scriptSlot)
        mPrevScriptAssetID = scriptSlot->assetID;
}

void AddComponentScriptCommand::redo()
{
    mScene.create_component_script_slot(mCompID, mScriptAssetID);
}

void AddComponentScriptCommand::undo()
{
    // update Scene
    if (mPrevScriptAssetID != 0)
    {
        ComponentScriptSlot* slot = mScene.get_component_script_slot(mCompID);
        LD_ASSERT(slot);
        slot->assetID = mPrevScriptAssetID;
    }
    else
    {
        mScene.destroy_component_script_slot(mCompID);
    }
}

} // namespace LD