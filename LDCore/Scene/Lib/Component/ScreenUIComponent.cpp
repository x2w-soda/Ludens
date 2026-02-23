#include <Ludens/Asset/AssetType/UITemplateAsset.h>
#include <Ludens/Profiler/Profiler.h>

#include "../UIDriver.h"
#include "ScreenUIComponent.h"

namespace LD {

static UIWindow create_window_container(SceneObj* scene)
{
    UILayoutInfo layoutI{};
    layoutI.sizeX = UISize::fit();
    layoutI.sizeY = UISize::fit();
    UIWindowInfo windowI{};
    UIWorkspace space = scene->screenUI.workspace();
    UIWindow window = space.create_float_window(layoutI, windowI, nullptr);
    window.set_pos(Vec2(0.0f, 0.0f));
    return window;
}

bool load_screen_ui_component(SceneObj* scene, ScreenUIComponent* ui, AssetID uiTemplateID, std::string& err)
{
    LD_PROFILE_SCOPE;

    ComponentBase* base = ui->base;

    UITemplateAsset asset = (UITemplateAsset)scene->assetManager.get_asset(uiTemplateID);
    if (!asset || asset.get_type() != ASSET_TYPE_UI_TEMPLATE)
        return false;

    ui->uiWindow = create_window_container(scene);

    if (!ui->uiWindow || !asset.load_ui_subtree(ui->uiWindow, nullptr, nullptr))
        return false;

    ui->uiTemplateID = uiTemplateID;

    base->flags |= COMPONENT_FLAG_LOADED_BIT;
    return true;
}

bool clone_screen_ui_component(SceneObj* scene, ComponentBase** dstData, ComponentBase** srcData, std::string& err)
{
    LD_PROFILE_SCOPE;

    Scene::ScreenUI srcUI((ScreenUIComponent*)srcData);
    Scene::ScreenUI dstUI((ScreenUIComponent*)dstData);
    LD_ASSERT(srcUI && dstUI);

    AssetID uiTemplateID = srcUI.get_ui_template_asset();

    return load_screen_ui_component(scene, (ScreenUIComponent*)dstData, uiTemplateID, err);
}

bool unload_screen_ui_component(SceneObj* scene, ComponentBase** data, std::string& err)
{
    LD_PROFILE_SCOPE;

    ScreenUIComponent* ui = (ScreenUIComponent*)data;

    UIWorkspace space = scene->screenUI.workspace();
    space.destroy_window(ui->uiWindow);
    ui->uiWindow = {};

    ComponentBase* base = *data;
    base->flags &= ~COMPONENT_FLAG_LOADED_BIT;

    return true;
}

bool startup_screen_ui_component(SceneObj* scene, ComponentBase** data, std::string& err)
{
    LD_PROFILE_SCOPE;

    auto* ui = (ScreenUIComponent*)data;

    UITemplateAsset asset = (UITemplateAsset)scene->assetManager.get_asset(ui->uiTemplateID);
    LD_ASSERT(asset && asset.get_type() == ASSET_TYPE_UI_TEMPLATE);

    LuaState luaState = scene->luaContext.get_lua_state();
    ui->uiDriver = heap_new<UIDriver>(MEMORY_USAGE_SCENE);

    if (!ui->uiDriver->connect(ui->uiWindow, luaState, asset.get_lua_source(), err))
        return false;

    if (!ui->uiDriver->attach(err))
        return false;

    return true;
}

bool cleanup_screen_ui_component(SceneObj* scene, ComponentBase** data, std::string& err)
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
    std::string err;

    return load_screen_ui_component(sScene, mUI, uiTemplateID, err);
}

bool Scene::ScreenUI::set_ui_template_asset(AssetID uiTemplateID)
{
    LD_ASSERT_COMPONENT_LOADED(mData);

    UITemplateAsset asset = (UITemplateAsset)sScene->assetManager.get_asset(uiTemplateID);
    if (!asset || asset.get_type() != ASSET_TYPE_UI_TEMPLATE)
        return false;

    mUI->uiWindow = create_window_container(sScene);

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