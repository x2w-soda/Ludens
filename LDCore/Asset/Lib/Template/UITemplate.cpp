#include <Ludens/Asset/Template/UITemplate.h>
#include <Ludens/DSA/HashMap.h>
#include <Ludens/DSA/Vector.h>
#include <Ludens/Header/Assert.h>
#include <Ludens/Header/Types.h>
#include <Ludens/Profiler/Profiler.h>

#include <cstdint>

#include "UITemplateObj.h"

namespace LD {

static_assert(LD::IsTrivial<UIScrollWidgetTemplate>);
static_assert(LD::IsTrivial<UISliderWidgetTemplate>);
static_assert(LD::IsTrivial<UIToggleWidgetTemplate>);
static_assert(LD::IsTrivial<UIPanelWidgetTemplate>);
static_assert(LD::IsTrivial<UIImageWidgetTemplate>);
static_assert(LD::IsTrivial<UITextWidgetTemplate>);

UITemplateEntry::UITemplateEntry(UIWidgetType type)
    : type(type)
{
    switch (type)
    {
    case UI_WIDGET_BUTTON:
        new (&button)(UIButtonWidgetTemplate);
        break;
    }
}

UITemplateEntry::~UITemplateEntry()
{
    switch (type)
    {
    case UI_WIDGET_BUTTON:
        (&button)->~UIButtonWidgetTemplate();
        break;
    }
}

UITemplateEntry* UITemplateObj::allocate_entry(UIWidgetType type)
{
    UITemplateEntry* entry = (UITemplateEntry*)entryPA.allocate();
    new (entry) UITemplateEntry(type);

    return entry;
}

void UITemplateObj::reset()
{
    for (auto it = entryPA.begin(); it; ++it)
    {
        UITemplateEntry* entry = (UITemplateEntry*)it.data();
        entry->~UITemplateEntry();
        entryPA.free(entry);
    }

    entries.clear();
    hierarchy.clear();
    LA.free();
}

class UITemplateSaver
{
public:
    /// @brief Save a widget subtree as template.
    void save(UITemplateObj* obj, UIWidget subtree, UITemplateOnSaveCallback callback, void* user);

public:
    static void save_ui_button(UITemplateSaver& saver, UITemplateEntry& entry, UIWidget widget);
    static void save_ui_panel(UITemplateSaver& saver, UITemplateEntry& entry, UIWidget widget);
    static void save_ui_image(UITemplateSaver& saver, UITemplateEntry& entry, UIWidget widget);
    static void save_ui_text(UITemplateSaver& saver, UITemplateEntry& entry, UIWidget widget);

private:
    uint32_t save_widget_subtree(UIWidget subtree);

private:
    void* mUser = nullptr;
    UITemplateObj* mTmpl = nullptr;
    UITemplateOnSaveCallback mCallback;
};

class UITemplateLoader
{
public:
    /// @brief Load a widget subtree from template.
    UIWidget load(UITemplateObj* obj, UIWidget parent, UITemplateOnLoadCallback callback, void* user);

public:
    static UIWidget load_ui_button(UITemplateLoader& loader, const UITemplateEntry& entry, UIWidget parent);
    static UIWidget load_ui_panel(UITemplateLoader& loader, const UITemplateEntry& entry, UIWidget parent);
    static UIWidget load_ui_image(UITemplateLoader& loader, const UITemplateEntry& entry, UIWidget parent);
    static UIWidget load_ui_text(UITemplateLoader& loader, const UITemplateEntry& entry, UIWidget parent);

private:
    UIWidget load_widget_subtree(UIWidget parent, uint32_t id);

private:
    UITemplateObj* mTmpl = nullptr;
    UITemplateOnLoadCallback mCallback;
    UIContext mCtx = {};
    void* mUser = nullptr;
};

// clang-format off
struct
{
    UIWidgetType type;
    void (*save)(UITemplateSaver& saver, UITemplateEntry& entry, UIWidget widget);
    UIWidget (*load)(UITemplateLoader& loader, const UITemplateEntry& entry, UIWidget parent);
} sUITemplateTable[] = {
    {UI_WIDGET_WINDOW,    nullptr,                            nullptr,                          },
    {UI_WIDGET_SCROLL,    nullptr,                            nullptr,                          },
    {UI_WIDGET_BUTTON,    &UITemplateSaver::save_ui_button,   &UITemplateLoader::load_ui_button },
    {UI_WIDGET_SLIDER,    nullptr,                            nullptr,                          },
    {UI_WIDGET_TOGGLE,    nullptr,                            nullptr,                          },
    {UI_WIDGET_PANEL,     &UITemplateSaver::save_ui_panel,    &UITemplateLoader::load_ui_panel, },
    {UI_WIDGET_IMAGE,     &UITemplateSaver::save_ui_image,    &UITemplateLoader::load_ui_image, },
    {UI_WIDGET_TEXT,      &UITemplateSaver::save_ui_text,     &UITemplateLoader::load_ui_text,  },
    {UI_WIDGET_TEXT_EDIT, nullptr,                            nullptr,                          },
};
// clang-format on

static_assert(sizeof(sUITemplateTable) / sizeof(*sUITemplateTable) == UI_WIDGET_TYPE_COUNT);

void UITemplateSaver::save_ui_button(UITemplateSaver& saver, UITemplateEntry& entry, UIWidget widget)
{
    LD_ASSERT(widget && widget.get_type() == UI_WIDGET_BUTTON);

    UIButtonWidget button = (UIButtonWidget)widget;
    entry.button.text = std::string(button.get_button_text());
    entry.button.info.text = entry.button.text.c_str();
}

void UITemplateSaver::save_ui_panel(UITemplateSaver& saver, UITemplateEntry& entry, UIWidget widget)
{
    LD_ASSERT(widget && widget.get_type() == UI_WIDGET_PANEL);

    UIPanelWidget panel = (UIPanelWidget)widget;
    entry.panel.info.color = *panel.panel_color();
}

void UITemplateSaver::save_ui_image(UITemplateSaver& saver, UITemplateEntry& entry, UIWidget widget)
{
    LD_ASSERT(widget && widget.get_type() == UI_WIDGET_IMAGE);

    Rect* imageRect = (Rect*)saver.mTmpl->LA.allocate(sizeof(Rect));
    UIImageWidget image = (UIImageWidget)widget;
    *imageRect = image.get_image_rect();

    entry.image.info.image = {};
    entry.image.info.rect = imageRect;
}

void UITemplateSaver::save_ui_text(UITemplateSaver& saver, UITemplateEntry& entry, UIWidget widget)
{
    LD_ASSERT(widget && widget.get_type() == UI_WIDGET_TEXT);

    UITextWidget text = (UITextWidget)widget;
    const char* cstr = text.get_text();

    entry.text.info = {};
    entry.text.info.bgColor = nullptr;
    entry.text.info.fontSize = *text.font_size();

    if (cstr)
    {
        size_t len = strlen(cstr);
        char* dup = (char*)saver.mTmpl->LA.allocate(len + 1);
        memcpy(dup, cstr, len);
        dup[len] = '\0';
        entry.text.info.cstr = dup;
    }
}

UIWidget UITemplateLoader::load_ui_button(UITemplateLoader& loader, const UITemplateEntry& entry, UIWidget parent)
{
    LD_ASSERT(parent && entry.type == UI_WIDGET_BUTTON);

    return parent.node().add_button(entry.layout, entry.button.info, nullptr);
}

UIWidget UITemplateLoader::load_ui_panel(UITemplateLoader& loader, const UITemplateEntry& entry, UIWidget parent)
{
    LD_ASSERT(parent && entry.type == UI_WIDGET_PANEL);

    return parent.node().add_panel(entry.layout, entry.panel.info, nullptr);
}

UIWidget UITemplateLoader::load_ui_image(UITemplateLoader& loader, const UITemplateEntry& entry, UIWidget parent)
{
    LD_ASSERT(parent && entry.type == UI_WIDGET_IMAGE);

    return parent.node().add_image(entry.layout, entry.image.info, nullptr);
}

UIWidget UITemplateLoader::load_ui_text(UITemplateLoader& loader, const UITemplateEntry& entry, UIWidget parent)
{
    LD_ASSERT(parent && entry.type == UI_WIDGET_TEXT);

    return parent.node().add_text(entry.layout, entry.text.info, nullptr);
}

void UITemplateSaver::save(UITemplateObj* obj, UIWidget subtree, UITemplateOnSaveCallback callback, void* user)
{
    mTmpl = obj;
    mTmpl->reset();
    mCallback = callback;
    mUser = user;

    save_widget_subtree(subtree);
}

uint32_t UITemplateSaver::save_widget_subtree(UIWidget root)
{
    UIWidgetType type = root.get_type();
    uint32_t entryIdx = (uint32_t)mTmpl->entries.size();
    UITemplateEntry* entry = mTmpl->allocate_entry(type);

    root.get_name(entry->name);
    root.get_layout(entry->layout);

    mTmpl->entries.push_back(entry);

    // save root widget in a single entry
    sUITemplateTable[(int)root.get_type()].save(*this, *entry, root);
    if (mCallback)
        mCallback(root, *entry, mUser);

    std::vector<UIWidget> children;
    root.node().get_children(children);

    for (UIWidget child : children)
    {
        uint32_t childID = save_widget_subtree(child);
        mTmpl->hierarchy[entryIdx].push_back(childID);
    }

    return entryIdx;
}

UIWidget UITemplateLoader::load(UITemplateObj* obj, UIWidget parent, UITemplateOnLoadCallback callback, void* user)
{
    mTmpl = obj;
    mCallback = callback;
    mUser = user;
    mCtx = UIContext(parent.node().get_context());

    if (mTmpl->entries.empty())
        return {};

    return load_widget_subtree(parent, 0);
}

UIWidget UITemplateLoader::load_widget_subtree(UIWidget parent, uint32_t id)
{
    const UITemplateEntry* entry = mTmpl->entries[id];
    LD_ASSERT(entry);

    // load root widget from a single entry
    UIWidget root = sUITemplateTable[(int)entry->type].load(*this, *entry, parent);
    LD_ASSERT(root);

    root.set_name(entry->name);

    if (mCallback)
        mCallback(root, *entry, mUser);

    for (uint32_t childID : mTmpl->hierarchy[id])
    {
        load_widget_subtree(root, childID);
    }

    return root;
}

//
// Public API
//

UITemplate UITemplate::create()
{
    auto* obj = heap_new<UITemplateObj>(MEMORY_USAGE_MISC);

    PoolAllocatorInfo paI{};
    paI.blockSize = sizeof(UITemplateEntry);
    paI.pageSize = 16;
    paI.isMultiPage = true;
    paI.usage = MEMORY_USAGE_MISC;
    obj->entryPA = PoolAllocator::create(paI);

    LinearAllocatorInfo laI{};
    laI.capacity = 512;
    laI.isMultiPage = true;
    laI.usage = MEMORY_USAGE_MISC;
    obj->LA = LinearAllocator::create(laI);

    return UITemplate(obj);
}

void UITemplate::destroy(UITemplate tmpl)
{
    auto* obj = tmpl.unwrap();

    obj->reset();

    LinearAllocator::destroy(obj->LA);
    PoolAllocator::destroy(obj->entryPA);

    heap_delete<UITemplateObj>(obj);
}

void UITemplate::save(UIWidget subtree, UITemplateOnSaveCallback callback, void* user)
{
    LD_PROFILE_SCOPE;

    UITemplateSaver saver;
    saver.save(mObj, subtree, callback, user);
}

UIWidget UITemplate::load(UIWidget parent, UITemplateOnLoadCallback callback, void* user)
{
    LD_PROFILE_SCOPE;

    UITemplateLoader loader;
    return loader.load(mObj, parent, callback, user);
}

} // namespace LD