#include <LudensEditor/AssetSelectWindow/AssetSelectWindow.h>
#include <LudensEditor/EditorWidget/EUIRow.h>

namespace LD {

struct AssetSelectWindowObj : EditorWindowObj
{
    EditorTheme theme = {};
    AssetType filterType = ASSET_TYPE_ENUM_COUNT;
    UIScrollStorage uiEntryScroll;
    int selectedRowIndex = -1;
    AssetID selectedAssetID = {};

    AssetSelectWindowObj(const EditorWindowInfo& info)
        : EditorWindowObj(info)
    {
    }

    EditorWindowType get_type() override { return EDITOR_WINDOW_ASSET_SELECT; }
    void on_imgui(float delta) override;
    inline AssetImporter get_asset_importer() { return mCtx.get_asset_importer(); }
};

void AssetSelectWindowObj::on_imgui(float delta)
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

    theme = mCtx.get_theme();

    std::string str;
    UILayoutInfo layoutI = theme.make_vbox_layout();
    layoutI.childPadding = UIPadding(theme.get_child_pad_large());
    layoutI.childGap = theme.get_child_gap_large();
    layoutI.sizeX = UISize::grow();
    layoutI.sizeY = UISize::grow();

    ui_workspace_begin();
    ui_push_window(ui_workspace_name());
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

    int btn = eui_row_btn_btn("cancel", "select");
    if (btn == 1)
        mShouldClose = true;
    else if (btn == 2 && 0 <= selectedRowIndex && selectedRowIndex < entries.size())
    {
        mShouldClose = true;
        selectedAssetID = entries[selectedRowIndex].get_id();
    }

    ui_pop();
    ui_pop_window();
    ui_workspace_end();
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
