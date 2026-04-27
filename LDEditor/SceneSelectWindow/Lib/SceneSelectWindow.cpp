#include <Ludens/DSA/IndexTable.h>
#include <Ludens/Project/Project.h>
#include <LudensEditor/EditorContext/EditorContextUtil.h>
#include <LudensEditor/EditorWidget/EUIButton.h>
#include <LudensEditor/EditorWidget/EUIRow.h>
#include <LudensEditor/SceneSelectWindow/SceneSelectWindow.h>

namespace LD {

struct SceneSelectWindowObj;

class SceneSelectWindowBottomBar
{
public:
    void update(SceneSelectWindowObj* obj);

private:
    EUIButtonRow<2> mButtonRow;
};

struct SceneSelectWindowObj : EditorWindowObj
{
    SceneSelectWindowBottomBar bottomBar;
    IndexTable<EUILabelRow, MEMORY_USAGE_UI> uiEntryRows;
    UIScrollData uiEntryScroll;
    Vector<SUIDEntry> sceneEntries;
    int selectedRowIndex = -1;

    SceneSelectWindowObj(const EditorWindowInfo& info)
        : EditorWindowObj(info)
    {
    }

    void update();
};

void SceneSelectWindowObj::update()
{
    sceneEntries = ctx.get_project_scene_entries();

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
    for (size_t i = 0; i < sceneEntries.size(); i++)
    {
        std::string newPath;
        const SUIDEntry& entry = sceneEntries[i];
        if (uiEntryRows[i]->update(entry.path.c_str(), i, i == selectedRowIndex, newPath))
            selectedRowIndex = i;

        if (!newPath.empty())
        {
            auto* actionE = (EditorActionRenameSceneEvent*)ctx.enqueue_event(EDITOR_EVENT_TYPE_ACTION_RENAME_SCENE);
            actionE->sceneID = entry.id;
            actionE->newPath = newPath;
        }
    }
    eui_pop_row_scroll();

    bottomBar.update(this);

    ui_pop();

    end_update_window();
}

void SceneSelectWindowBottomBar::update(SceneSelectWindowObj* obj)
{
    mButtonRow.label[0] = "Cancel";
    mButtonRow.label[1] = "Select";
    mButtonRow.isEnabled[1] = 0 <= obj->selectedRowIndex && obj->selectedRowIndex < obj->sceneEntries.size();

    int btnPressed = mButtonRow.update();
    if (btnPressed == 1)
        obj->shouldClose = true;
    else if (btnPressed == 2)
    {
        obj->shouldClose = true;

        if (0 <= obj->selectedRowIndex && obj->selectedRowIndex < obj->sceneEntries.size())
        {
            auto* actionE = (EditorActionOpenSceneEvent*)obj->ctx.enqueue_event(EDITOR_EVENT_TYPE_ACTION_OPEN_SCENE);
            actionE->sceneID = obj->sceneEntries[obj->selectedRowIndex].id;
        }
    }
}

//
// Public API
//

EditorWindow SceneSelectWindow::create(const EditorWindowInfo& windowI)
{
    auto* obj = heap_new<SceneSelectWindowObj>(MEMORY_USAGE_UI, windowI);

    return EditorWindow(obj);
}

void SceneSelectWindow::destroy(EditorWindow window)
{
    auto* obj = static_cast<SceneSelectWindowObj*>(window.unwrap());

    heap_delete<SceneSelectWindowObj>(obj);
}

void SceneSelectWindow::update(EditorWindowObj* base, const EditorUpdateTick& tick)
{
    auto* obj = static_cast<SceneSelectWindowObj*>(base);

    (void)tick;

    obj->update();
}

} // namespace LD