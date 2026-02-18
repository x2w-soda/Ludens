#include <Ludens/Asset/AssetType/UITemplateAsset.h>
#include <Ludens/Profiler/Profiler.h>

#include "../UIDriver.h"
#include "ScreenUIComponent.h"

namespace LD {

bool load_screen_ui_component(SceneObj* scene, ScreenUIComponent* ui, AssetID uiTemplateID)
{
    LD_PROFILE_SCOPE;

    ComponentBase* base = ui->base;

    UITemplateAsset asset = (UITemplateAsset)scene->assetManager.get_asset(uiTemplateID);
    if (!asset || asset.get_type() != ASSET_TYPE_UI_TEMPLATE)
        return false;

    UILayoutInfo layoutI{};
    UIWindowInfo windowI{};
    UIWorkspace space = scene->screenUI.workspace();
    ui->uiWindow = space.create_window(layoutI, windowI, nullptr);

    if (!ui->uiWindow || !asset.load_ui_subtree(ui->uiWindow, nullptr, nullptr))
        return false;

    ui->uiTemplateID = uiTemplateID;

    base->flags |= COMPONENT_FLAG_LOADED_BIT;
    return true;
}

bool clone_screen_ui_component(SceneObj* scene, ComponentBase** dstData, ComponentBase** srcData)
{
    LD_PROFILE_SCOPE;

    Scene::ScreenUI srcUI((ScreenUIComponent*)srcData);
    Scene::ScreenUI dstUI((ScreenUIComponent*)dstData);
    LD_ASSERT(srcUI && dstUI);

    AssetID uiTemplateID = srcUI.get_ui_template_asset();

    return load_screen_ui_component(scene, (ScreenUIComponent*)dstData, uiTemplateID);
}

void unload_screen_ui_component(SceneObj* scene, ComponentBase** data)
{
    LD_PROFILE_SCOPE;

    ScreenUIComponent* ui = (ScreenUIComponent*)data;

    UIWorkspace space = scene->screenUI.workspace();
    space.destroy_window(ui->uiWindow);
    ui->uiWindow = {};

    ComponentBase* base = *data;
    base->flags &= ~COMPONENT_FLAG_LOADED_BIT;
}

void startup_screen_ui_component(SceneObj* scene, ComponentBase** data)
{
    LD_PROFILE_SCOPE;

    auto* ui = (ScreenUIComponent*)data;

    UITemplateAsset asset = (UITemplateAsset)scene->assetManager.get_asset(ui->uiTemplateID);
    LD_ASSERT(asset && asset.get_type() == ASSET_TYPE_UI_TEMPLATE);

    LuaState luaState = scene->luaContext.get_lua_state();
    ui->uiDriver = heap_new<UIDriver>(MEMORY_USAGE_SCENE);

    std::string err;
    bool ok = ui->uiDriver->connect(ui->uiWindow, luaState, asset.get_lua_source(), err);
    LD_ASSERT(ok);

    ok = ui->uiDriver->attach(err);
    LD_ASSERT(ok);
}

void cleanup_screen_ui_component(SceneObj* scene, ComponentBase** data)
{
    LD_PROFILE_SCOPE;

    auto* ui = (ScreenUIComponent*)data;
    LD_ASSERT(ui);

    std::string err;
    bool ok = ui->uiDriver->detach(err);
    LD_ASSERT(ok);

    ok = ui->uiDriver->disconnect();
    LD_ASSERT(ok);

    heap_delete<UIDriver>(ui->uiDriver);
    ui->uiDriver = nullptr;
}

Scene::ScreenUI::ScreenUI(Component comp)
{
    if (comp && comp.type() == COMPONENT_TYPE_SCREEN_UI)
    {
        mData = comp.data();
        mUI = (ScreenUIComponent*)mData;
    }
}

Scene::ScreenUI::ScreenUI(ScreenUIComponent* comp)
{
    if (comp && comp->base && comp->base->cuid)
    {
        mData = (ComponentBase**)comp;
        mUI = comp;
    }
}

bool Scene::ScreenUI::load(AssetID uiTemplateID)
{
    return load_screen_ui_component(sScene, mUI, uiTemplateID);
}

bool Scene::ScreenUI::set_ui_template_asset(AssetID uiTemplateID)
{
    LD_ASSERT_COMPONENT_LOADED(mData);

    UITemplateAsset asset = (UITemplateAsset)sScene->assetManager.get_asset(uiTemplateID);
    if (!asset || asset.get_type() != ASSET_TYPE_UI_TEMPLATE)
        return false;

    UILayoutInfo layoutI{};
    UIWindowInfo windowI{};
    UIWorkspace space = sScene->screenUI.workspace();
    mUI->uiWindow = space.create_window(layoutI, windowI, nullptr);

    if (!mUI->uiWindow || !asset.load_ui_subtree(mUI->uiWindow, nullptr, nullptr))
        return false;

    mUI->uiTemplateID = uiTemplateID;

    return true;
}

AssetID Scene::ScreenUI::get_ui_template_asset()
{
    LD_ASSERT_COMPONENT_LOADED(mData);

    return mUI->uiTemplateID;
}

} // namespace LD