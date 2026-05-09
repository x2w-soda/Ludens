#include <Ludens/DSA/IndexTable.h>
#include <LudensEditor/AssetSelectWindow/AssetSelectWindow.h>
#include <LudensEditor/EditorContext/EditorContextUtil.h>
#include <LudensEditor/EditorWidget/EUIButton.h>
#include <LudensEditor/EditorWidget/EUIRow.h>

namespace LD {

struct AssetSelectWindowObj;

class AssetSelectWindowTopBar
{
public:
    void update(AssetSelectWindowObj* obj);

private:
    UIPanelData mPanel;
    EUIButton mButton;
};

class AssetSelectWindowBottomBar
{
public:
    void update(AssetSelectWindowObj* obj);

private:
    void confirm_select_asset(AssetSelectWindowObj* obj, AssetID id);
    void confirm_create_script(AssetSelectWindowObj* obj, const String& requestPath);

    EUIButtonRow<2> mButtonRow;
};

struct AssetSelectWindowObj : EditorWindowObj
{
    AssetSelectWindowTopBar topBar;
    AssetSelectWindowBottomBar bottomBar;
    AssetSelectWindowMode mode = ASSET_SELECT_WINDOW_MODE_DEFAULT;
    AssetType filterType = ASSET_TYPE_ENUM_COUNT;
    AssetRegistry assetRegistry = {};
    SUID component = 0;
    IndexTable<EUILabelRow, MEMORY_USAGE_UI> uiEntryRows;
    UIScrollData uiEntryScroll;
    uint32_t componentAssetSlotIndex = 0;
    Vector<AssetEntry> entries;
    EUIAssetPathEditRow uiPathEditRow;
    int selectedRowIndex = -1;
    String assetPath;

    AssetSelectWindowObj(const EditorWindowInfo& info)
        : EditorWindowObj(info)
    {
    }

    void update();
    void update_mode_default();
    void update_mode_new_script();
};

void AssetSelectWindowObj::update()
{
    AssetManager AM = AssetManager::get();
    assetRegistry = AM.get_asset_registry(); // nullable if no project is loaded

    if (assetRegistry) // per-frame registry query... not sure how this scales
    {
        if (filterType == ASSET_TYPE_ENUM_COUNT)
            assetRegistry.get_all_entries(entries);
        else
            assetRegistry.get_entries_by_type(entries, filterType);
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

    topBar.update(this);

    if (mode == ASSET_SELECT_WINDOW_MODE_DEFAULT)
        update_mode_default();
    else
        update_mode_new_script();

    bottomBar.update(this);

    ui_pop();

    end_update_window();
}

void AssetSelectWindowObj::update_mode_default()
{
    String newPath;

    eui_push_row_scroll(&uiEntryScroll);
    for (size_t i = 0; i < entries.size(); i++)
    {
        AssetEntry entry = entries[i];
        if (uiEntryRows[i]->update(entry.get_path().c_str(), i, i == selectedRowIndex, newPath))
            selectedRowIndex = i;

        if (!newPath.empty())
        {
            auto* actionE = (EditorActionRenameAssetEvent*)ctx.enqueue_event(EDITOR_EVENT_TYPE_ACTION_RENAME_ASSET);
            actionE->assetID = entry.get_id();
            actionE->newPath = newPath;
        }
    }
    eui_pop_row_scroll();
}

void AssetSelectWindowObj::update_mode_new_script()
{
    if (uiPathEditRow.update(assetRegistry, assetPath))
        ;
}

void AssetSelectWindowTopBar::update(AssetSelectWindowObj* obj)
{
    EditorTheme theme = obj->ctx.get_theme();
    float height = theme.get_text_row_height();

    UILayoutInfo layoutI(UISize::grow(), UISize::fixed(height), UI_AXIS_X);
    ui_push_panel(nullptr);
    ui_top_layout(layoutI);

    {
        ui_push_panel(nullptr);
        ui_top_layout_size_x(UISize::grow());
        ui_pop();
        switch (obj->mode)
        {
        case ASSET_SELECT_WINDOW_MODE_NEW_SCRIPT:
            if (mButton.update(EDITOR_ICON_ADD_FOLDER, "Select Existing"))
                obj->mode = ASSET_SELECT_WINDOW_MODE_DEFAULT;
            break;
        case ASSET_SELECT_WINDOW_MODE_DEFAULT:
        default:
            if (mButton.update(EDITOR_ICON_ADD_FOLDER, "New Script"))
                obj->mode = ASSET_SELECT_WINDOW_MODE_NEW_SCRIPT;
            break;
        }
    }

    ui_pop();
}

void AssetSelectWindowBottomBar::update(AssetSelectWindowObj* obj)
{
    bool isDefaultMode = obj->mode == ASSET_SELECT_WINDOW_MODE_DEFAULT;

    mButtonRow.label[0] = "Cancel";
    mButtonRow.label[1] = isDefaultMode ? "Select" : "Create";
    mButtonRow.isEnabled[1] = true;
    if (isDefaultMode)
        mButtonRow.isEnabled[1] = 0 <= obj->selectedRowIndex && obj->selectedRowIndex < obj->entries.size();
    else
        mButtonRow.isEnabled[1] = !obj->assetPath.empty() && obj->uiPathEditRow.is_path_valid();
    int btnPressed = mButtonRow.update();

    if (btnPressed == 1)
        obj->shouldClose = true;
    else if (btnPressed == 2)
    {
        obj->shouldClose = true;

        if (isDefaultMode)
        {
            if (0 <= obj->selectedRowIndex && obj->selectedRowIndex < obj->entries.size())
            {
                AssetID assetID = obj->entries[obj->selectedRowIndex].get_id();
                confirm_select_asset(obj, assetID);
            }
        }
        else
        {
            confirm_create_script(obj, obj->assetPath);
        }
    }
}

void AssetSelectWindowBottomBar::confirm_select_asset(AssetSelectWindowObj* obj, AssetID assetID)
{
    if (!obj->component)
        return;

    if (obj->filterType == ASSET_TYPE_LUA_SCRIPT)
        EditorContextUtil::set_component_script(obj->ctx, obj->component, assetID);
    else
        EditorContextUtil::set_component_asset(obj->ctx, obj->component, assetID, obj->componentAssetSlotIndex);
}

void AssetSelectWindowBottomBar::confirm_create_script(AssetSelectWindowObj* obj, const String& requestPath)
{
    // TODO: check params again
    if (!obj->assetRegistry)
        return;

    String err;
    Asset asset = EditorContextUtil::create_lua_script_asset(obj->ctx, requestPath, err);

    if (asset)
        EditorContextUtil::set_component_script(obj->ctx, obj->component, asset.get_id());
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

    (void)tick;

    obj->update();
}

void AssetSelectWindow::mode_hint(EditorWindowObj* base, EditorWindowMode modeHint)
{
    auto* obj = static_cast<AssetSelectWindowObj*>(base);

    obj->mode = (AssetSelectWindowMode)modeHint;
}

void AssetSelectWindow::set_filter(AssetType type)
{
    mObj->filterType = type;
}

void AssetSelectWindow::set_component(SUID comp)
{
    mObj->component = comp;
}

void AssetSelectWindow::set_component_asset_slot_index(uint32_t index)
{
    mObj->componentAssetSlotIndex = index;
}

} // namespace LD
