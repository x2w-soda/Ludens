#include <LudensBuilder/AssetBuilder/AssetImporter.h>
#include <LudensEditor/AssetImportWindow/AssetImportWindow.h>
#include <LudensEditor/EditorWidget/EUIRow.h>

#include <LudensBuilder/AssetBuilder/AssetSource/TextureAssetSource.h>

namespace LD {

struct AssetImportWindowObj : EditorWindowObj
{
    AssetImportInfo* importInfo = {};
    UITextEditData dstFilePath;
    UITextEditData dstURI;

    AssetImportWindowObj(const EditorWindowInfo& info)
        : EditorWindowObj(info)
    {
    }

    void update(float delta);
    inline AssetImporter get_asset_importer() { return ctx.get_asset_importer(); }
};

void AssetImportWindowObj::update(float delta)
{
    (void)delta;

    std::string str;
    UILayoutInfo layoutI = theme.make_vbox_layout();
    layoutI.childPadding = UIPadding(theme.get_child_pad_large());
    layoutI.childGap = theme.get_child_gap_large();
    layoutI.sizeX = UISize::grow();
    layoutI.sizeY = UISize::grow();

    begin_update_window();

    ui_push_panel(nullptr);
    ui_top_layout(layoutI);

    if (eui_row_label_text_edit("Imported File Path", &dstFilePath, str))
        importInfo->dstRelPath = str;

    if (eui_row_label_text_edit("Asset URI", &dstURI, str))
        importInfo->dstURI = str;

    if (importInfo && importInfo->type == ASSET_TYPE_TEXTURE_2D)
    {
        // eui_texture_2d_asset_import(texture2DAIS, importInfo);
    }

    static UIButtonData sBtn[2];
    sBtn[0].text = "Cancel";
    sBtn[1].text = "Import";
    int btnPressed = eui_row_btn_btn(sBtn, sBtn + 1);
    if (btnPressed == 1)
        shouldClose = true;
    else if (btnPressed == 2 && importInfo)
    {
        auto* actionE = (EditorActionImportAssetsEvent*)ctx.enqueue_event(EDITOR_EVENT_TYPE_ACTION_IMPORT_ASSETS);
        actionE->batch.resize(1);
        actionE->batch[0] = importInfo;
        importInfo->dstRelPath = dstFilePath.get_text();
        importInfo->dstURI = dstURI.get_text();
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

    // TODO:
    LD_UNREACHABLE;

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

    obj->update(tick.delta);
}

void AssetImportWindow::set_type(AssetType type)
{
    AssetImporter importer = mObj->get_asset_importer();

    if (mObj->importInfo)
        importer.free_import_info(mObj->importInfo);

    mObj->importInfo = importer.allocate_import_info(type);
}

void AssetImportWindow::set_source_path(const std::string& srcPath)
{
    LD_ASSERT(mObj->importInfo && mObj->importInfo->type == ASSET_TYPE_TEXTURE_2D);

    ((Texture2DAssetImportInfo*)mObj->importInfo)->srcPath = srcPath;
    mObj->dstURI.set_text(FS::Path(srcPath).stem().string());
}

} // namespace LD