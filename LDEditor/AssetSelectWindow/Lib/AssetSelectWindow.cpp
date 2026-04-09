#include <LudensEditor/AssetSelectWindow/AssetSelectWindow.h>
#include <LudensEditor/EditorWidget/EUIRow.h>

namespace LD {

struct AssetSelectWindowObj : EditorWindowObj
{
    AssetType filterType = ASSET_TYPE_ENUM_COUNT;
    UIScrollStorage uiEntryScroll;
    int selectedRowIndex = -1;
    AssetID selectedAssetID = {};

    AssetSelectWindowObj(const EditorWindowInfo& info)
        : EditorWindowObj(info)
    {
    }

    void update(float delta);
    inline AssetImporter get_asset_importer() { return ctx.get_asset_importer(); }
};

void AssetSelectWindowObj::update(float delta)
{
    (void)delta;

    Vector<AssetEntry> entries;
    AssetManager AM = AssetManager::get();
    AssetRegistry AR = AM.get_asset_registry(); // nullable if no project is loaded

    if (AR) // per-frame registry query... not sure how this scales
    {
        if (filterType == ASSET_TYPE_ENUM_COUNT)
            AR.get_all_entries(entries);
        else
            AR.get_entries_by_type(entries, filterType);
    }

    std::string str;
    UILayoutInfo layoutI = theme.make_vbox_layout();
    layoutI.childPadding = UIPadding(theme.get_child_pad_large());
    layoutI.childGap = theme.get_child_gap_large();
    layoutI.sizeX = UISize::grow();
    layoutI.sizeY = UISize::grow();

    begin_update_window();

    ui_push_panel(nullptr);
    ui_top_layout(layoutI);

    eui_push_row_scroll(&uiEntryScroll);
    for (size_t i = 0; i < entries.size(); i++)
    {
        AssetEntry entry = entries[i];
        if (eui_row_label(i, entry.get_uri().c_str(), i == selectedRowIndex))
            selectedRowIndex = i;
    }
    eui_pop_row_scroll();

    static UIButtonStorage sBtn[2];
    sBtn[0].text = "Cancel";
    sBtn[1].text = "Select";
    int btnPressed = eui_row_btn_btn(sBtn, sBtn + 1);
    if (btnPressed == 1)
        shouldClose = true;
    else if (btnPressed == 2 && 0 <= selectedRowIndex && selectedRowIndex < entries.size())
    {
        shouldClose = true;
        selectedAssetID = entries[selectedRowIndex].get_id();
    }

    ui_pop();

    end_update_window();
}

//
// Public API
//

EditorWindow AssetSelectWindow::create(const EditorWindowInfo& windowI)
{
    auto* obj = heap_new<AssetSelectWindowObj>(MEMORY_USAGE_UI, windowI);

    return EditorWindow(obj);
}

void AssetSelectWindow::destroy(EditorWindow window)
{
    auto* obj = static_cast<AssetSelectWindowObj*>(window.unwrap());

    heap_delete<AssetSelectWindowObj>(obj);
}

void AssetSelectWindow::update(EditorWindowObj* base, const EditorUpdateTick& tick)
{
    auto* obj = static_cast<AssetSelectWindowObj*>(base);

    obj->update(tick.delta);
}

void AssetSelectWindow::set_filter(AssetType type)
{
    mObj->filterType = type;
}

bool AssetSelectWindow::has_selected_asset(AssetID& id)
{
    if (mObj->selectedAssetID)
    {
        id = mObj->selectedAssetID;
        mObj->selectedAssetID = {};
        return true;
    }

    return false;
}

} // namespace LD
