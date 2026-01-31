#include <Ludens/Memory/Memory.h>
#include <Ludens/Profiler/Profiler.h>
#include <LudensEditor/CreateComponentWindow/CreateComponentWindow.h>
#include <LudensEditor/SelectionWindow/SelectionWindow.h>

#include <LudensEditor/EditorUI/EditorDialog.h>
#include <LudensEditor/EditorUI/EditorUIDialog.h>

namespace LD {

enum DialogType
{
    DIALOG_NONE = 0,
    DIALOG_PROJECT_SETTINGS,
    DIALOG_OPEN_SCENE,
    DIALOG_OPEN_PROJECT,
    DIALOG_SELECT_ASSET,
    DIALOG_SELECT_SCRIPT,
    DIALOG_CREATE_COMPONENT,
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
    WindowID get_dialog_window_id();

    void dialog_project_settings();
    void dialog_open_scene();
    void dialog_open_project();
    void dialog_select_asset(const EditorEvent* e);
    void dialog_select_script();
    void dialog_create_component(const EditorEvent* e);

private:
    EditorWindow get_or_create_dialog(EditorWindowType type);

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

    if (!mDialog || !mDialog.get_id())
    {
        mDialogType = DIALOG_NONE;
        return;
    }

    if (mDialog.should_close())
    {
        EditorDialog::destroy(mDialog);
        mDialog = {};
        return;
    }

    FS::Path selectedPath{};
    SelectionWindow selectW{};

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
    case DIALOG_OPEN_PROJECT:
        selectW = (SelectionWindow)mDialog.get_editor_window(EDITOR_WINDOW_SELECTION);
        if (selectW && selectW.has_selected(selectedPath))
        {
            mDialogType = DIALOG_NONE;
            mCtx.action_open_project(selectedPath);
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

WindowID EditorUIDialogObj::get_dialog_window_id()
{
    return mDialog ? mDialog.get_id() : 0;
}

void EditorUIDialogObj::dialog_project_settings()
{
    LD_ASSERT(mDialogType == DIALOG_NONE);
    mDialogType = DIALOG_PROJECT_SETTINGS;

    get_or_create_dialog(EDITOR_WINDOW_PROJECT_SETTINGS);
}

void EditorUIDialogObj::dialog_open_scene()
{
    LD_ASSERT(mDialogType == DIALOG_NONE);
    mDialogType = DIALOG_OPEN_SCENE;

    SelectionWindow selectionW = (SelectionWindow)get_or_create_dialog(EDITOR_WINDOW_SELECTION);
    selectionW.show(mCtx.get_project_directory(), "toml");
}

void EditorUIDialogObj::dialog_open_project()
{
    LD_ASSERT(mDialogType == DIALOG_NONE);
    mDialogType = DIALOG_OPEN_PROJECT;

    SelectionWindow selectionW = (SelectionWindow)get_or_create_dialog(EDITOR_WINDOW_SELECTION);
    selectionW.show(mCtx.get_project_directory(), "toml");
}

void EditorUIDialogObj::dialog_select_asset(const EditorEvent* e)
{
    LD_ASSERT(e && e->type == EDITOR_EVENT_TYPE_REQUEST_COMPONENT_ASSET);
    LD_ASSERT(mDialogType == DIALOG_NONE);
    mDialogType = DIALOG_SELECT_ASSET;

    const auto* event = (const EditorRequestComponentAssetEvent*)e;
    mSubjectCompID = event->component;

    SelectionWindow selectionW = (SelectionWindow)get_or_create_dialog(EDITOR_WINDOW_SELECTION);
    selectionW.show(mCtx.get_project_directory(), "lda");
}

void EditorUIDialogObj::dialog_select_script()
{
    LD_ASSERT(mDialogType == DIALOG_NONE);
    mDialogType = DIALOG_SELECT_SCRIPT;

    SelectionWindow selectionW = (SelectionWindow)get_or_create_dialog(EDITOR_WINDOW_SELECTION);
    selectionW.show(mCtx.get_project_directory(), "lua");
}

void EditorUIDialogObj::dialog_create_component(const EditorEvent* e)
{
    LD_ASSERT(e && e->type == EDITOR_EVENT_TYPE_REQUEST_CREATE_COMPONENT);
    LD_ASSERT(mDialogType == DIALOG_NONE);
    mDialogType = DIALOG_CREATE_COMPONENT;
    
    CreateComponentWindow createCompW = (CreateComponentWindow)get_or_create_dialog(EDITOR_WINDOW_CREATE_COMPONENT);

    const auto* event = (const EditorRequestCreateComponentEvent*)e;
    createCompW.set_parent_component(event->parent);
}

EditorWindow EditorUIDialogObj::get_or_create_dialog(EditorWindowType type)
{
    EditorWindow editorW;

    if (mDialog)
    {
        if ((editorW = mDialog.get_editor_window(type)))
            return editorW;

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

    editorW = mDialog.get_editor_window(type);
    LD_ASSERT(editorW);

    return editorW;
}

void EditorUIDialogObj::on_editor_event(const EditorEvent* event, void* user)
{
    auto* obj = (EditorUIDialogObj*)user;

    if (event->category != EDITOR_EVENT_CATEGORY_REQUEST)
        return;

    switch (event->type)
    {
    case EDITOR_EVENT_TYPE_REQUEST_PROJECT_SETTINGS:
        obj->dialog_project_settings();
        break;
    case EDITOR_EVENT_TYPE_REQUEST_COMPONENT_ASSET:
        obj->dialog_select_asset(event);
        break;
    case EDITOR_EVENT_TYPE_REQUEST_CREATE_COMPONENT:
        obj->dialog_create_component(event);
        break;
    case EDITOR_EVENT_TYPE_REQUEST_OPEN_SCENE:
        obj->dialog_open_scene();
        break;
    case EDITOR_EVENT_TYPE_REQUEST_OPEN_PROJECT:
        obj->dialog_open_project();
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