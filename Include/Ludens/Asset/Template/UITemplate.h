#pragma once

#include <Ludens/Asset/Asset.h>
#include <Ludens/Header/Handle.h>
#include <Ludens/System/FileSystem.h>
#include <Ludens/UI/UIContext.h>
#include <Ludens/UI/UIWidget.h>

#include <string>

namespace LD {

struct UIScrollWidgetTemplate
{
    UIScrollWidgetInfo info;
};

struct UIButtonWidgetTemplate
{
    UIButtonWidgetInfo info;
};

struct UISliderWidgetTemplate
{
    UISliderWidgetInfo info;
};

struct UIToggleWidgetTemplate
{
    UIToggleWidgetInfo info;
};

struct UIPanelWidgetTemplate
{
    UIPanelWidgetInfo info;
};

struct UIImageWidgetTemplate
{
    UIImageWidgetInfo info;
    Rect imageRect;           /// if size is not zero, the area in image to be rendered.
    AssetID texture2DAssetID; /// the Texture2DAsset used by image widget.
};

struct UITextWidgetTemplate
{
    UITextWidgetInfo info;
    AssetID fontAssetID; /// the FontAsset used by text widget.
};

/// @brief Template information to instantiate a UIWidget.
struct UITemplateEntry
{
    UIWidgetType type;
    UILayoutInfo layout;
    std::string name;

    union
    {
        UIScrollWidgetTemplate scroll;
        UIButtonWidgetTemplate button;
        UISliderWidgetTemplate slider;
        UIToggleWidgetTemplate toggle;
        UIPanelWidgetTemplate panel;
        UIImageWidgetTemplate image;
        UITextWidgetTemplate text;
    };
};

typedef bool (*UITemplateOnSaveCallback)(UIWidget widget, UITemplateEntry& tmpl, void* user);
typedef bool (*UITemplateOnLoadCallback)(UIWidget widget, const UITemplateEntry& tmpl, void* user);

struct UITemplate : Handle<struct UITemplateObj>
{
    static UITemplate create();

    static void destroy(UITemplate tmpl);

    /// @brief Save some widget subtree as template.
    void save(UIWidget subtree, UITemplateOnSaveCallback callback, void* user);

    /// @brief Instantiates a widget subtree under a parent widget.
    UIWidget load(UIWidget parent, UITemplateOnLoadCallback callback, void* user);
};

} // namespace LD