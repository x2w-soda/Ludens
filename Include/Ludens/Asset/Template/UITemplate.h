#pragma once

#include <Ludens/Asset/Asset.h>
#include <Ludens/Header/Handle.h>
#include <Ludens/System/FileSystem.h>
#include <Ludens/UI/UIContext.h>
#include <Ludens/UI/UIWidget.h>
#include <Ludens/UI/Widget/UIButtonWidget.h>
#include <Ludens/UI/Widget/UIImageWidget.h>
#include <Ludens/UI/Widget/UIPanelWidget.h>
#include <Ludens/UI/Widget/UIScrollWidget.h>
#include <Ludens/UI/Widget/UISliderWidget.h>
#include <Ludens/UI/Widget/UITextEditWidget.h>
#include <Ludens/UI/Widget/UITextWidget.h>
#include <Ludens/UI/Widget/UIToggleWidget.h>

#include <string>

namespace LD {

struct UIScrollTemplate
{
    UIScrollData storage;
};

struct UIButtonTemplate
{
    UIButtonData storage;
};

struct UISliderTemplate
{
    UISliderData storage;
};

struct UIToggleTemplate
{
    UIToggleData storage;
};

struct UIPanelTemplate
{
    UIPanelData storage;
};

struct UIImageTemplate
{
    UIImageData storage;
    AssetID texture2DAssetID; /// the Texture2DAsset used by image widget.
};

struct UITextTemplate
{
    UITextData storage;
    AssetID fontAssetID; /// the FontAsset used by text widget.
};

/// @brief Template information to instantiate a UIWidget.
struct UITemplateEntry
{
    const UIWidgetType type;
    UILayoutInfo layout;
    std::string name;

    union
    {
        UIScrollTemplate scroll;
        UIButtonTemplate button;
        UISliderTemplate slider;
        UIToggleTemplate toggle;
        UIPanelTemplate panel;
        UIImageTemplate image;
        UITextTemplate text;
    };

    UITemplateEntry() = delete;
    UITemplateEntry(UIWidgetType type);
    UITemplateEntry(const UITemplateEntry&) = delete;
    ~UITemplateEntry();

    UITemplateEntry& operator=(const UITemplateEntry&) = delete;
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