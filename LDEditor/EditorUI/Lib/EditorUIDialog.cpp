#include <Ludens/Memory/Memory.h>
#include <Ludens/Profiler/Profiler.h>
#include <LudensEditor/EditorUI/EditorDialog.h>
#include <LudensEditor/EditorUI/EditorUIDialog.h>
#include <LudensEditor/SelectComponentWindow/SelectComponentWindow.h>
#include <LudensEditor/SelectionWindow/SelectionWindow.h>

namespace LD {

enum DialogType
{
    DIALOG_NONE = 0,
    DIALOG_OPEN_SCENE,
    DIALOG_SELECT_ASSET,
    DIALOG_SELECT_COMPONENT,
    DIALOG_SELECT_SCRIPT,
};

class EditorUIDialogObj
{
public:
    EditorUIDialogObj(const EditorUIDialogInfo& mainI);
    EditorUIDialogObj(const EditorUIDialogObj&) = delete;
    ~EditorUIDialogObj();

    EditorUIDialogObj& operator=(const EditorUIDialogObj&) = delete;

    inline void render(ScreenRenderComponent renderer)
    {
        if (mDialog)
            mDialog.render(renderer);
    }

    void update(float delta);
    void create_dialog(DialogType type);
    WindowID get_dialog_window_id();

private:
    void get_or_create_dialog(EditorWindowType type);

    static void on_editor_event(const EditorEvent* event, void* user);

private:
    EditorContext mCtx{};
    EditorDialog mDialog{};
    FontAtlas mFontAtlas{};
    RImage mFontAtlasImage{};
    DialogType mDialogType = DIALOG_NONE;
    CUID mSubjectCompID = 0;
};

EditorUIDialogObj::EditorUIDialogObj(const EditorUIDialogInfo& mainI)
    : mCtx(mainI.ctx), mFontAtlas(mainI.fontAtlas), mFontAtlasImage(mainI.fontAtlasImage)
{
    mCtx.add_observer(&EditorUIDialogObj::on_editor_event, this);
}

EditorUIDialogObj::~EditorUIDialogObj()
{
    if (mDialog)
        EditorDialog::destroy(mDialog);
}

void EditorUIDialogObj::update(float delta)
{
    LD_PROFILE_SCOPE;

    FS::Path selectedPath{};
    SelectionWindow selectW{};

    if (!mDialog || !mDialog.get_id())
    {
        mDialogType = DIALOG_NONE;
        return;
    }

    // updates the UIContext in the EditorDialog (a separate OS-level Window)
    mDialog.update(delta);

    // generate actions or events
    switch (mDialogType)
    {
    case DIALOG_OPEN_SCENE:
        selectW = (SelectionWindow)mDialog.get_editor_window(EDITOR_WINDOW_SELECTION);
        if (selectW && selectW.has_selected(selectedPath))
        {
            mDialogType = DIALOG_NONE;
            mCtx.action_open_scene(selectedPath);
        }
        break;
    case DIALOG_SELECT_ASSET:
        selectW = (SelectionWindow)mDialog.get_editor_window(EDITOR_WINDOW_SELECTION);
        if (selectW && selectW.has_selected(selectedPath))
        {
            mDialogType = DIALOG_NONE;
            std::string stem = selectedPath.stem().string();
            AssetManager AM = mCtx.get_asset_manager();
            AUID assetID = AM.get_id_from_name(stem.c_str(), nullptr);
            mCtx.action_set_component_asset(mSubjectCompID, assetID);
        }
        break;
    case DIALOG_SELECT_SCRIPT:
        selectW = (SelectionWindow)mDialog.get_editor_window(EDITOR_WINDOW_SELECTION);
        if (selectW && selectW.has_selected(selectedPath))
        {
            mDialogType = DIALOG_NONE;
            if (!mCtx.get_component_base(mSubjectCompID))
                return; // component out of date

            AssetType type;
            AssetManager AM = mCtx.get_asset_manager();
            std::string stem = selectedPath.stem().string();
            AUID scriptAssetID = AM.get_id_from_name(stem.c_str(), &type);
            if (scriptAssetID == 0 || type != ASSET_TYPE_LUA_SCRIPT)
                return; // script asset out of date

            mCtx.action_add_component_script(mSubjectCompID, scriptAssetID);
        }
        break;
    default:
        break;
    }
}

void EditorUIDialogObj::create_dialog(DialogType type)
{
    LD_ASSERT(mDialogType == DIALOG_NONE);

    mDialogType = type;

    SelectionWindow selectionW{};
    SelectComponentWindow selectCompW{};

    switch (type)
    {
    case DIALOG_OPEN_SCENE:
        get_or_create_dialog(EDITOR_WINDOW_SELECTION);
        selectionW = (SelectionWindow)mDialog.get_editor_window(EDITOR_WINDOW_SELECTION);
        selectionW.show(mCtx.get_project_directory(), "toml");
        break;
    case DIALOG_SELECT_ASSET:
        get_or_create_dialog(EDITOR_WINDOW_SELECTION);
        selectionW = (SelectionWindow)mDialog.get_editor_window(EDITOR_WINDOW_SELECTION);
        selectionW.show(mCtx.get_project_directory(), "lda");
        break;
    case DIALOG_SELECT_COMPONENT:
        get_or_create_dialog(EDITOR_WINDOW_SELECT_COMPONENT);
        LD_UNREACHABLE;
        break;
    case DIALOG_SELECT_SCRIPT:
        get_or_create_dialog(EDITOR_WINDOW_SELECTION);
        selectionW = (SelectionWindow)mDialog.get_editor_window(EDITOR_WINDOW_SELECTION);
        selectionW.show(mCtx.get_project_directory(), "lua");
        break;
    default:
        LD_UNREACHABLE;
    }
}

WindowID EditorUIDialogObj::get_dialog_window_id()
{
    return mDialog ? mDialog.get_id() : 0;
}

void EditorUIDialogObj::get_or_create_dialog(EditorWindowType type)
{
    if (mDialog)
    {
        if (mDialog.get_editor_window(type))
            return;

        EditorDialog::destroy(mDialog);
        mDialogType = DIALOG_NONE;
        mDialog = {};
    }

    EditorDialogInfo dialogI{};
    dialogI.ctx = mCtx;
    dialogI.extent = Vec2(512, 512);
    dialogI.fontAtlas = mFontAtlas;
    dialogI.fontAtlasImage = mFontAtlasImage;
    dialogI.type = type;
    mDialog = EditorDialog::create(dialogI);
}

void EditorUIDialogObj::on_editor_event(const EditorEvent* event, void* user)
{
    auto* obj = (EditorUIDialogObj*)user;

    switch (event->type)
    {
    case EDITOR_EVENT_TYPE_REQUEST_COMPONENT_ASSET:
        obj->create_dialog(DIALOG_SELECT_ASSET);
        break;
    default:
        break;
    }
}

//
// Public API
//

EditorUIDialog EditorUIDialog::create(const EditorUIDialogInfo& modalI)
{
    auto* obj = heap_new<EditorUIDialogObj>(MEMORY_USAGE_UI, modalI);

    return EditorUIDialog(obj);
}

void EditorUIDialog::destroy(EditorUIDialog modal)
{
    auto* obj = modal.unwrap();

    heap_delete<EditorUIDialogObj>(obj);
}

void EditorUIDialog::render(ScreenRenderComponent renderer)
{
    mObj->render(renderer);
}

void EditorUIDialog::update(float delta)
{
    mObj->update(delta);
}

WindowID EditorUIDialog::get_dialog_window_id()
{
    return mObj->get_dialog_window_id();
}

} // namespace LD