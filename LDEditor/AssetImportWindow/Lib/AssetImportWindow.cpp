#include <LudensBuilder/AssetBuilder/AssetImporter.h>
#include <LudensEditor/AssetImportWindow/AssetImportWindow.h>
#include <LudensEditor/EditorWidget/EUIRow.h>

#include <LudensBuilder/AssetBuilder/AssetSource/TextureAssetSource.h>

namespace LD {

struct AssetImportWindowObj : EditorWindowObj
{
    AssetImportInfo* importInfo = {};
    EUIAssetPathEditRow uiDstPathRow;
    EUIButtonRow<2> uiButtonRow;
    std::string dstPath;

    AssetImportWindowObj(const EditorWindowInfo& info)
        : EditorWindowObj(info)
    {
    }

    void update();
};

void AssetImportWindowObj::update()
{
    std::string str;
    UILayoutInfo layoutI = theme.make_vbox_layout();
    layoutI.childPadding = UIPadding(theme.get_child_pad_large());
    layoutI.childGap = theme.get_child_gap_large();
    layoutI.sizeX = UISize::grow();
    layoutI.sizeY = UISize::grow();

    AssetRegistry assetReg = AssetManager::get().get_asset_registry();

    begin_update_window();

    ui_push_panel(nullptr);
    ui_top_layout(layoutI);

    if (uiDstPathRow.update(assetReg, dstPath))
        importInfo->dstPath = dstPath;

    uiButtonRow.label[0] = "Cancel";
    uiButtonRow.label[1] = "Import";
    uiButtonRow.isEnabled[1] = !dstPath.empty() && uiDstPathRow.is_path_valid();
    int btnPressed = uiButtonRow.update();
    if (btnPressed == 1)
        shouldClose = true;
    else if (btnPressed == 2 && importInfo)
    {
        auto* actionE = (EditorActionImportAssetsEvent*)ctx.enqueue_event(EDITOR_EVENT_TYPE_ACTION_IMPORT_ASSETS);
        actionE->batch.resize(1);
        actionE->batch[0] = importInfo;
        importInfo->dstPath = dstPath;
        importInfo = nullptr;
    }

    ui_pop();

    end_update_window();
}

//
// Public API
//

EditorWindow AssetImportWindow::create(const EditorWindowInfo& windowI)
{
    auto* obj = heap_new<AssetImportWindowObj>(MEMORY_USAGE_UI, windowI);

    return EditorWindow(obj);
}

void AssetImportWindow::destroy(EditorWindow window)
{
    auto* obj = static_cast<AssetImportWindowObj*>(window.unwrap());

    heap_delete<AssetImportWindowObj>(obj);
}

void AssetImportWindow::update(EditorWindowObj* base, const EditorUpdateTick& tick)
{
    auto* obj = static_cast<AssetImportWindowObj*>(base);

    (void)tick;

    obj->update();
}

void AssetImportWindow::set_type(AssetType type)
{
    EditorContextAssetInterface assetI = mObj->ctx.asset_interface();

    if (mObj->importInfo)
        assetI.free_asset_import_info(mObj->importInfo);

    mObj->importInfo = assetI.alloc_asset_import_info(type);
}

void AssetImportWindow::set_source_file(const FS::Path& srcFilePath)
{
    if (!mObj->importInfo)
        return;

    (void)mObj->importInfo->set_src_files(1, &srcFilePath);
    mObj->dstPath = FS::Path(srcFilePath).stem().string();
}

} // namespace LD