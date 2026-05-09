#include <Ludens/Asset/AssetType/UITemplateAsset.h>
#include <Ludens/Profiler/Profiler.h>
#include <Ludens/Scene/Component/ScreenUIView.h>

#include "../UIDriver.h"
#include "ScreenUIComponent.h"

namespace LD {

#if 0
static UIWindow create_window_container(SceneObj* scene)
{
    UILayoutInfo layoutI{};
    layoutI.sizeX = UISize::fit();
    layoutI.sizeY = UISize::fit();
    UIWindowInfo windowI{};
    UIWorkspace space = scene->active->screenUI.workspace();
    UIWindow window = space.create_float_window(layoutI, windowI, nullptr);
    window.set_pos(Vec2(0.0f, 0.0f));
    return window;
}

void init_screen_ui_component(ComponentBase** dstData)
{
    ScreenUIComponent* dstScreenUI = (ScreenUIComponent*)dstData;
    dstScreenUI->uiDriver = nullptr;
    dstScreenUI->uiTemplateID = 0;
    dstScreenUI->uiWindow = {};
}

bool load_screen_ui_component(SceneObj* scene, ScreenUIComponent* ui, AssetID uiTemplateID, String& err)
{
    LD_PROFILE_SCOPE;

    ComponentBase* base = ui->base;

    AssetManager AM = AssetManager::get();
    UITemplateAsset asset = (UITemplateAsset)AM.get_asset(uiTemplateID);
    if (!asset || asset.get_type() != ASSET_TYPE_UI_TEMPLATE)
        return false;

    ui->uiWindow = create_window_container(scene);

    if (!ui->uiWindow || !asset.load_ui_subtree(ui->uiWindow, nullptr, nullptr))
        return false;

    ui->uiTemplateID = uiTemplateID;

    ui->uiWindow.layout();

    return true;
}

bool clone_screen_ui_component(SceneObj* scene, ComponentBase** dstData, ComponentBase** srcData, String& err)
{
    LD_PROFILE_SCOPE;

    ScreenUIView srcUI((ScreenUIComponent*)srcData);
    ScreenUIView dstUI((ScreenUIComponent*)dstData);
    LD_ASSERT(srcUI && dstUI);

    AssetID uiTemplateID = srcUI.get_ui_template_asset();

    return load_screen_ui_component(scene, (ScreenUIComponent*)dstData, uiTemplateID, err);
}

bool unload_screen_ui_component(SceneObj* scene, ComponentBase** data, String& err)
{
    LD_PROFILE_SCOPE;

    ScreenUIComponent* ui = (ScreenUIComponent*)data;

    UIWorkspace space = scene->active->screenUI.workspace();
    space.destroy_window(ui->uiWindow);
    ui->uiWindow = {};

    ComponentBase* base = *data;

    return true;
}

bool startup_screen_ui_component(SceneObj* scene, ComponentBase** data, String& err)
{
    LD_PROFILE_SCOPE;

    auto* ui = (ScreenUIComponent*)data;

    AssetManager AM = AssetManager::get();
    UITemplateAsset asset = (UITemplateAsset)AM.get_asset(ui->uiTemplateID);
    LD_ASSERT(asset && asset.get_type() == ASSET_TYPE_UI_TEMPLATE);

    LuaState luaState = scene->active->lua.get_lua_state();
    ui->uiDriver = heap_new<UIDriver>(MEMORY_USAGE_SCENE);

    if (!ui->uiDriver->connect(ui->uiWindow, luaState, asset.get_lua_source(), err))
        return false;

    if (!ui->uiDriver->attach(err))
        return false;

    return true;
}

bool cleanup_screen_ui_component(SceneObj* scene, ComponentBase** data, String& err)
{
    LD_PROFILE_SCOPE;

    auto* ui = (ScreenUIComponent*)data;
    LD_ASSERT(ui);

    if (!ui->uiDriver)
        return true; // already clean

    if (!ui->uiDriver->detach(err))
        return false;

    if (!ui->uiDriver->disconnect())
        return false;

    heap_delete<UIDriver>(ui->uiDriver);
    ui->uiDriver = nullptr;

    return true;
}

ScreenUIView::ScreenUIView(ComponentView comp)
{
    if (comp && comp.type() == COMPONENT_TYPE_SCREEN_UI)
    {
        mData = comp.data();
        mUI = (ScreenUIComponent*)mData;
    }
}

ScreenUIView::ScreenUIView(ScreenUIComponent* comp)
{
    if (comp && comp->base && comp->base->cuid)
    {
        mData = (ComponentBase**)comp;
        mUI = comp;
    }
}

bool ScreenUIView::load(AssetID uiTemplateID)
{
    String err;

    return load_screen_ui_component(sScene, mUI, uiTemplateID, err);
}

bool ScreenUIView::set_ui_template_asset(AssetID uiTemplateID)
{
    LD_ASSERT(mUI->uiWindow);

    AssetManager AM = AssetManager::get();
    UITemplateAsset asset = (UITemplateAsset)AM.get_asset(uiTemplateID);
    if (!asset || asset.get_type() != ASSET_TYPE_UI_TEMPLATE)
        return false;

    mUI->uiWindow = create_window_container(sScene);

    if (!mUI->uiWindow || !asset.load_ui_subtree(mUI->uiWindow, nullptr, nullptr))
        return false;

    mUI->uiTemplateID = uiTemplateID;

    return true;
}

AssetID ScreenUIView::get_ui_template_asset()
{
    return mUI->uiTemplateID;
}
#else
void init_screen_ui_component(ComponentBase** dstData)
{
    LD_UNREACHABLE;
}
bool load_screen_ui_component(SceneObj* scene, ScreenUIComponent* ui, AssetID uiTemplateID, String& err)
{
    return false;
}
bool clone_screen_ui_component(SceneObj* scene, ComponentBase** dstData, ComponentBase** srcData, String& err)
{
    return false;
}
bool unload_screen_ui_component(SceneObj* scene, ComponentBase** data, String& err)
{
    return false;
}
bool startup_screen_ui_component(SceneObj* scene, ComponentBase** data, String& err)
{
    return false;
}

bool cleanup_screen_ui_component(SceneObj* scene, ComponentBase** data, String& err)
{
    return false;
}
AssetID ScreenUIView::get_ui_template_asset()
{
    return 0;
}
#endif
} // namespace LD