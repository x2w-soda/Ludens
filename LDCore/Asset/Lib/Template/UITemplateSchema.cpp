#include <Ludens/Asset/Template/UITemplateSchema.h>
#include <Ludens/Header/Assert.h>
#include <Ludens/Header/Version.h>
#include <Ludens/Media/Format/TOML.h>
#include <Ludens/Profiler/Profiler.h>

#include "UITemplateObj.h"

namespace LD {

/// @brief Saves a UITemplate to TOML schema.
class UITemplateSchemaSaver
{
public:
    /// @brief Save template as TOML schema.
    void save_to_schema(UITemplateObj* obj, TOMLDocument doc);

    static void save_ui_panel_toml(UITemplateSchemaSaver& saver, const UITemplateEntry& entry, TOMLValue widgetTOML);
    static void save_ui_image_toml(UITemplateSchemaSaver& saver, const UITemplateEntry& entry, TOMLValue widgetTOML);

private:
    void save_widget_subtree_toml(uint32_t idx);
    void save_widget_layout_toml(const UILayoutInfo& layout, TOMLValue widgetTOML);

private:
    UITemplateObj* mTmpl = nullptr;
    TOMLValue mWidgetArrayTOML;
    TOMLValue mHierarchyTOML;
};

/// @brief Loads a UITemplate from TOML schema.
class UITemplateSchemaLoader
{
public:
    /// @brief Load template from TOML schema.
    void load_from_schema(UITemplateObj* obj, TOMLDocument doc);

    static void load_ui_panel_toml(UITemplateSchemaLoader& loader, UITemplateEntry& entry, TOMLValue widgetTOML);
    static void load_ui_image_toml(UITemplateSchemaLoader& loader, UITemplateEntry& entry, TOMLValue widgetTOML);

private:
    void load_widget_toml(TOMLValue widgetTOML);
    bool load_layout_toml(UILayoutInfo& layout, TOMLValue layoutTOML);
    bool load_layout_size_toml(UISize& size, TOMLValue sizeTOML);
    bool load_layout_child_align_toml(UIAlign& align, TOMLValue alignTOML);
    bool load_layout_child_padding_toml(UIPadding& padding, TOMLValue paddingTOML);

private:
    UITemplateObj* mTmpl = nullptr;
};

// clang-format off
struct
{
    UIWidgetType type;
    void (*save_toml)(UITemplateSchemaSaver& saver, const UITemplateEntry& entry, TOMLValue widgetTOML);
    void (*load_toml)(UITemplateSchemaLoader& loader, UITemplateEntry& entry, TOMLValue widgetTOML);
} sUITemplateSchemaTable[] = {
    {UI_WIDGET_WINDOW,    nullptr,                                    nullptr},
    {UI_WIDGET_SCROLL,    nullptr,                                    nullptr},
    {UI_WIDGET_BUTTON,    nullptr,                                    nullptr},
    {UI_WIDGET_SLIDER,    nullptr,                                    nullptr},
    {UI_WIDGET_TOGGLE,    nullptr,                                    nullptr},
    {UI_WIDGET_PANEL,     &UITemplateSchemaSaver::save_ui_panel_toml, &UITemplateSchemaLoader::load_ui_panel_toml},
    {UI_WIDGET_IMAGE,     &UITemplateSchemaSaver::save_ui_image_toml, &UITemplateSchemaLoader::load_ui_image_toml},
    {UI_WIDGET_TEXT,      nullptr,                                    nullptr},
    {UI_WIDGET_TEXT_EDIT, nullptr,                                    nullptr},
};
// clang-format on

void UITemplateSchemaSaver::save_widget_subtree_toml(uint32_t idx)
{
    const UITemplateEntry* entry = mTmpl->entries[idx];

    TOMLValue widgetTOML = mWidgetArrayTOML.append(TOML_TYPE_TABLE);

    TOMLValue typeTOML = widgetTOML.set_key("type", TOML_TYPE_STRING);
    std::string typeStr(get_ui_widget_type_cstr(entry->type));
    typeTOML.set_string(typeStr);

    TOMLValue idxTOML = widgetTOML.set_key("index", TOML_TYPE_INT);
    idxTOML.set_u32(idx);

    // save root widget in a single entry
    save_widget_layout_toml(entry->layout, widgetTOML);
    sUITemplateSchemaTable[(int)entry->type].save_toml(*this, *entry, widgetTOML);

    std::string parentIdx = std::to_string(idx);
    TOMLValue childrenTOML = mHierarchyTOML.set_key(parentIdx.c_str(), TOML_TYPE_ARRAY);
    for (uint32_t childIdx : mTmpl->hierarchy[idx])
    {
        childrenTOML.append(TOML_TYPE_INT).set_u32(childIdx);

        save_widget_subtree_toml(idx);
    }
}

void UITemplateSchemaSaver::save_widget_layout_toml(const UILayoutInfo& layout, TOMLValue widgetTOML)
{
    TOMLValue layoutTOML = widgetTOML.set_key("layout", TOML_TYPE_TABLE);
    layoutTOML.format(TOML_FORMAT_TABLE_ONE_LINE);

    switch (layout.sizeX.type)
    {
    case UI_SIZE_FIXED:
        layoutTOML.set_key("size_x", TOML_TYPE_INT).set_i32((int32_t)layout.sizeX.extent);
        break;
    case UI_SIZE_GROW:
        layoutTOML.set_key("size_x", TOML_TYPE_STRING).set_string("grow");
        break;
    case UI_SIZE_WRAP:
        layoutTOML.set_key("size_x", TOML_TYPE_STRING).set_string("wrap");
        break;
    case UI_SIZE_FIT:
        layoutTOML.set_key("size_x", TOML_TYPE_STRING).set_string("fit");
        break;
    }

    switch (layout.sizeY.type)
    {
    case UI_SIZE_FIXED:
        layoutTOML.set_key("size_y", TOML_TYPE_INT).set_i32((int32_t)layout.sizeY.extent);
        break;
    case UI_SIZE_GROW:
        layoutTOML.set_key("size_y", TOML_TYPE_STRING).set_string("grow");
        break;
    case UI_SIZE_WRAP:
        layoutTOML.set_key("size_y", TOML_TYPE_STRING).set_string("wrap");
        break;
    case UI_SIZE_FIT:
        layoutTOML.set_key("size_y", TOML_TYPE_STRING).set_string("fit");
        break;
    }

    switch (layout.childAlignX)
    {
    case UI_ALIGN_BEGIN:
        layoutTOML.set_key("child_align_x", TOML_TYPE_STRING).set_string("begin");
        break;
    case UI_ALIGN_CENTER:
        layoutTOML.set_key("child_align_x", TOML_TYPE_STRING).set_string("center");
        break;
    case UI_ALIGN_END:
        layoutTOML.set_key("child_align_x", TOML_TYPE_STRING).set_string("end");
        break;
    }

    switch (layout.childAlignY)
    {
    case UI_ALIGN_BEGIN:
        layoutTOML.set_key("child_align_y", TOML_TYPE_STRING).set_string("begin");
        break;
    case UI_ALIGN_CENTER:
        layoutTOML.set_key("child_align_y", TOML_TYPE_STRING).set_string("center");
        break;
    case UI_ALIGN_END:
        layoutTOML.set_key("child_align_y", TOML_TYPE_STRING).set_string("end");
        break;
    }

    switch (layout.childAxis)
    {
    case UI_AXIS_X:
        layoutTOML.set_key("child_axis", TOML_TYPE_STRING).set_string("x");
        break;
    case UI_AXIS_Y:
        layoutTOML.set_key("child_axis", TOML_TYPE_STRING).set_string("y");
        break;
    }

    TOMLValue childPaddingTOML = layoutTOML.set_key("child_padding", TOML_TYPE_TABLE);
    childPaddingTOML.format(TOML_FORMAT_TABLE_ONE_LINE);
    childPaddingTOML.set_key("left", TOML_TYPE_FLOAT).set_f32(layout.childPadding.left);
    childPaddingTOML.set_key("right", TOML_TYPE_FLOAT).set_f32(layout.childPadding.right);
    childPaddingTOML.set_key("top", TOML_TYPE_FLOAT).set_f32(layout.childPadding.top);
    childPaddingTOML.set_key("bottom", TOML_TYPE_FLOAT).set_f32(layout.childPadding.bottom);

    layoutTOML.set_key("child_gap", TOML_TYPE_FLOAT).set_f32(layout.childGap);
}

void UITemplateSchemaSaver::save_to_schema(UITemplateObj* obj, TOMLDocument doc)
{
    mTmpl = obj;
    mWidgetArrayTOML = doc.set("widget", TOML_TYPE_ARRAY);
    mHierarchyTOML = doc.set("hierarchy", TOML_TYPE_TABLE);

    TOMLValue sceneTOML = doc.set("ludens_ui_template", TOML_TYPE_TABLE);
    sceneTOML.set_key("version_major", TOML_TYPE_INT).set_i32(LD_VERSION_MAJOR);
    sceneTOML.set_key("version_minor", TOML_TYPE_INT).set_i32(LD_VERSION_MINOR);
    sceneTOML.set_key("version_patch", TOML_TYPE_INT).set_i32(LD_VERSION_PATCH);

    if (!mTmpl->entries.empty())
        save_widget_subtree_toml(0);
}

void UITemplateSchemaSaver::save_ui_panel_toml(UITemplateSchemaSaver& saver, const UITemplateEntry& entry, TOMLValue widgetTOML)
{
    LD_ASSERT(entry.type == UI_WIDGET_TEXT);

    widgetTOML.set_key("color", TOML_TYPE_INT).set_u32((uint32_t)entry.panel.info.color);
}

void UITemplateSchemaSaver::save_ui_image_toml(UITemplateSchemaSaver& saver, const UITemplateEntry& entry, TOMLValue widgetTOML)
{
    LD_ASSERT(entry.type == UI_WIDGET_IMAGE);

    widgetTOML.set_key("texture_2d", TOML_TYPE_INT).set_u32(entry.image.texture2DAUID);
    TOMLValue rectTOML = widgetTOML.set_key("image_rect", TOML_TYPE_TABLE);
    TOMLUtil::save_rect_table(entry.image.imageRect, rectTOML);
}

void UITemplateSchemaLoader::load_ui_panel_toml(UITemplateSchemaLoader& loader, UITemplateEntry& entry, TOMLValue widgetTOML)
{
    LD_ASSERT(entry.type == UI_WIDGET_PANEL);

    uint32_t colorU32 = 0;

    TOMLValue colorTOML = widgetTOML.get_key("color", TOML_TYPE_INT);
    if (colorTOML)
        colorTOML.get_u32(colorU32);

    entry.panel.info.color = Color(colorU32);
}

void UITemplateSchemaLoader::load_ui_image_toml(UITemplateSchemaLoader& loader, UITemplateEntry& entry, TOMLValue widgetTOML)
{
    LD_ASSERT(entry.type == UI_WIDGET_IMAGE);

    entry.image.imageRect = {};
    entry.image.info.rect = &entry.image.imageRect;
    entry.image.info.image = {};

    TOMLValue rectTOML = widgetTOML.get_key("image_rect", TOML_TYPE_TABLE);
    TOMLUtil::load_rect_table(entry.image.imageRect, rectTOML);

    entry.image.texture2DAUID = 0;
    TOMLValue texture2DTOML = widgetTOML.get_key("texture_2d", TOML_TYPE_INT);
    if (texture2DTOML)
        texture2DTOML.get_u32(entry.image.texture2DAUID);
}

void UITemplateSchemaLoader::load_widget_toml(TOMLValue widgetTOML)
{
    uint32_t entryIdx;
    TOMLValue idxTOML = widgetTOML.get_key("index");
    if (!idxTOML || !idxTOML.get_u32(entryIdx))
        return;

    if (entryIdx >= mTmpl->entries.size())
        return;

    UITemplateEntry* entry = (UITemplateEntry*)mTmpl->entryPA.allocate();
    mTmpl->entries[entryIdx] = entry;

    std::string typeStr;
    TOMLValue typeTOML = widgetTOML.get_key("type");
    if (!typeTOML || !typeTOML.get_string(typeStr))
        return;

    TOMLValue layoutTOML = widgetTOML.get_key("layout", TOML_TYPE_TABLE);
    if (!layoutTOML || !load_layout_toml(entry->layout, layoutTOML))
        return;

    if (!get_ui_widget_type_from_cstr(entry->type, typeStr.c_str()))
        return;

    sUITemplateSchemaTable[(int)entry->type].load_toml(*this, *entry, widgetTOML);
}

bool UITemplateSchemaLoader::load_layout_toml(UILayoutInfo& layout, TOMLValue layoutTOML)
{
    layout = {};

    TOMLValue sizeTOML = layoutTOML.get_key("size_x");
    if (!sizeTOML || !load_layout_size_toml(layout.sizeX, sizeTOML))
        return false;

    sizeTOML = layoutTOML.get_key("size_y");
    if (!sizeTOML || !load_layout_size_toml(layout.sizeY, sizeTOML))
        return false;

    TOMLValue childAlignTOML = layoutTOML.get_key("child_align_x", TOML_TYPE_STRING);
    if (!childAlignTOML || !load_layout_child_align_toml(layout.childAlignX, childAlignTOML))
        return false;

    childAlignTOML = layoutTOML.get_key("child_align_y", TOML_TYPE_STRING);
    if (!childAlignTOML || !load_layout_child_align_toml(layout.childAlignY, childAlignTOML))
        return false;

    std::string str;
    TOMLValue childAxisTOML = layoutTOML.get_key("child_axis", TOML_TYPE_STRING);
    if (!childAxisTOML || !childAxisTOML.get_string(str))
        return false;

    if (str == "x")
        layout.childAxis = UI_AXIS_X;
    else if (str == "y")
        layout.childAxis = UI_AXIS_Y;
    else
        return false;

    TOMLValue childPaddingTOML = layoutTOML.get_key("child_padding", TOML_TYPE_TABLE);
    if (!childPaddingTOML || !load_layout_child_padding_toml(layout.childPadding, childPaddingTOML))
        return false;

    layoutTOML.set_key("child_gap", TOML_TYPE_FLOAT).set_f32(layout.childGap);

    return true;
}

bool UITemplateSchemaLoader::load_layout_size_toml(UISize& size, TOMLValue sizeTOML)
{
    std::string str;
    float f32;
    int32_t i32;

    if (sizeTOML.get_string(str))
    {
        if (str == "grow")
            size = UISize::grow();
        else if (str == "wrap")
            size = UISize::wrap();
        else if (str == "fit")
            size = UISize::fit();
        else
            return false;

        return true;
    }

    if (sizeTOML.get_f32(f32))
    {
        size = UISize::fixed(f32);
        return true;
    }

    if (sizeTOML.get_i32(i32))
    {
        size = UISize::fixed((float)i32);
        return true;
    }

    return false;
}

bool UITemplateSchemaLoader::load_layout_child_align_toml(UIAlign& align, TOMLValue alignTOML)
{
    std::string str;

    if (!alignTOML.get_string(str))
        return false;

    if (str == "begin")
    {
        align = UI_ALIGN_BEGIN;
        return true;
    }

    if (str == "center")
    {
        align = UI_ALIGN_CENTER;
        return true;
    }

    if (str == "end")
    {
        align = UI_ALIGN_END;
        return true;
    }

    return false;
}

bool UITemplateSchemaLoader::load_layout_child_padding_toml(UIPadding& padding, TOMLValue paddingTOML)
{
    TOMLValue padTOML = paddingTOML.get_key("left");
    if (!padTOML || !padTOML.get_f32(padding.left))
        return false;

    padTOML = paddingTOML.get_key("right");
    if (!padTOML || !padTOML.get_f32(padding.right))
        return false;

    padTOML = paddingTOML.get_key("top");
    if (!padTOML || !padTOML.get_f32(padding.top))
        return false;

    padTOML = paddingTOML.get_key("bottom");
    if (!padTOML || !padTOML.get_f32(padding.bottom))
        return false;

    return true;
}

void UITemplateSchemaLoader::load_from_schema(UITemplateObj* obj, TOMLDocument doc)
{
    mTmpl = obj;
    mTmpl->reset();

    TOMLValue sceneTOML = doc.get("ludens_ui_template");
    if (!sceneTOML || sceneTOML.get_type() != TOML_TYPE_TABLE)
        return;

    int32_t version;
    TOMLValue versionTOML = sceneTOML["version_major"];
    if (!versionTOML || !versionTOML.get_i32(version) || version != LD_VERSION_MAJOR)
        return;

    versionTOML = sceneTOML["version_minor"];
    if (!versionTOML || !versionTOML.get_i32(version) || version != LD_VERSION_MINOR)
        return;

    versionTOML = sceneTOML["version_patch"];
    if (!versionTOML || !versionTOML.get_i32(version) || version != LD_VERSION_PATCH)
        return;

    TOMLValue hierarchyTOML = doc.get("hierarchy");
    if (!hierarchyTOML || !hierarchyTOML.is_table_type())
        return;

    std::vector<std::string> keys;
    hierarchyTOML.get_keys(keys);
    for (const std::string& key : keys)
    {
        uint32_t parentIdx = static_cast<uint32_t>(std::stoul(key));
        TOMLValue childrenTOML = hierarchyTOML[key.c_str()];
        if (!childrenTOML || !childrenTOML.is_array_type())
            continue;

        int count = childrenTOML.get_size();
        for (int i = 0; i < count; i++)
        {
            uint32_t childIdx;
            if (!childrenTOML[i].get_u32(childIdx))
                continue;

            mTmpl->hierarchy[parentIdx].push_back(childIdx);
        }
    }

    TOMLValue widgetArrayTOML = doc.get("widget");
    if (!widgetArrayTOML || widgetArrayTOML.get_type() != TOML_TYPE_ARRAY)
        return;

    int widgetCount = widgetArrayTOML.get_size();
    mTmpl->entries.resize((size_t)widgetCount);

    for (int i = 0; i < widgetCount; i++)
    {
        TOMLValue widgetTOML = widgetArrayTOML[i];
        if (!widgetTOML || !widgetTOML.is_table_type())
            continue;

        load_widget_toml(widgetTOML);
    }
}

//
// Public API
//

void UITemplateSchema::load_ui_template_from_source(UITemplate tmpl, const void* source, size_t len)
{
    LD_PROFILE_SCOPE;

    TOMLDocument doc = TOMLDocument::create();

    std::string err;
    bool success = doc.parse((const char*)source, len, err);
    if (!success)
    {
        TOMLDocument::destroy(doc);
        return; // TODO: error handling path
    }

    UITemplateSchemaLoader loader;
    loader.load_from_schema(tmpl, doc);

    TOMLDocument::destroy(doc);
}

void UITemplateSchema::load_ui_template_from_file(UITemplate tmpl, const FS::Path& tomlPath)
{
    LD_PROFILE_SCOPE;

    TOMLDocument doc = TOMLDocument::create_from_file(tomlPath);

    UITemplateSchemaLoader loader;
    loader.load_from_schema(tmpl, doc);

    TOMLDocument::destroy(doc);
}

bool UITemplateSchema::save_ui_template(UITemplate tmpl, const FS::Path& savePath, std::string& err)
{
    LD_PROFILE_SCOPE;

    TOMLDocument doc = TOMLDocument::create();

    UITemplateSchemaSaver saver;
    saver.save_to_schema(tmpl, doc);

    std::string str;
    if (!doc.save_to_string(str))
    {
        TOMLDocument::destroy(doc);
        return false;
    }

    TOMLDocument::destroy(doc);
    return FS::write_file_and_swap_backup(savePath, str.size(), (const byte*)str.data(), err);
}

} // namespace LD