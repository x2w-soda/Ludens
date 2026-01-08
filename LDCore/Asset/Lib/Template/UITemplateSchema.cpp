#include <Ludens/Asset/Template/UITemplateSchema.h>
#include <Ludens/Header/Assert.h>
#include <Ludens/Header/Version.h>
#include <Ludens/Media/Format/TOML.h>
#include <Ludens/Profiler/Profiler.h>

#include "UITemplateObj.h"
#include "UITemplateSchemaKeys.h"

namespace LD {

/// @brief Saves a UITemplate to TOML schema.
class UITemplateSchemaSaver
{
public:
    UITemplateSchemaSaver() = default;
    UITemplateSchemaSaver(const UITemplateSchemaSaver&) = delete;
    ~UITemplateSchemaSaver();

    UITemplateSchemaSaver& operator=(const UITemplateSchemaSaver&) = delete;

    /// @brief Save template as TOML schema.
    bool save_template(UITemplateObj* obj, std::string& toml, std::string& err);

    static void save_ui_panel(UITemplateSchemaSaver& saver, const UITemplateEntry& entry);
    static void save_ui_image(UITemplateSchemaSaver& saver, const UITemplateEntry& entry);

private:
    void save_widget_subtree(uint32_t idx);
    void save_widget_layout(const UILayoutInfo& layout);

private:
    UITemplateObj* mTmpl = nullptr;
    TOMLWriter mWriter{};
};

/// @brief Loads a UITemplate from TOML schema.
class UITemplateSchemaLoader
{
public:
    UITemplateSchemaLoader() = default;
    UITemplateSchemaLoader(const UITemplateSchemaLoader&) = delete;
    ~UITemplateSchemaLoader();

    UITemplateSchemaLoader& operator=(const UITemplateSchemaLoader&) = delete;

    bool load_template(UITemplateObj* obj, const View& toml, std::string& err);

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
    TOMLDocument mDoc{};
};

// clang-format off
struct
{
    UIWidgetType type;
    void (*save_toml)(UITemplateSchemaSaver& saver, const UITemplateEntry& entry);
    void (*load_toml)(UITemplateSchemaLoader& loader, UITemplateEntry& entry, TOMLValue widgetTOML);
} sUITemplateSchemaTable[] = {
    {UI_WIDGET_WINDOW,    nullptr,                                    nullptr},
    {UI_WIDGET_SCROLL,    nullptr,                                    nullptr},
    {UI_WIDGET_BUTTON,    nullptr,                                    nullptr},
    {UI_WIDGET_SLIDER,    nullptr,                                    nullptr},
    {UI_WIDGET_TOGGLE,    nullptr,                                    nullptr},
    {UI_WIDGET_PANEL,     &UITemplateSchemaSaver::save_ui_panel, &UITemplateSchemaLoader::load_ui_panel_toml},
    {UI_WIDGET_IMAGE,     &UITemplateSchemaSaver::save_ui_image, &UITemplateSchemaLoader::load_ui_image_toml},
    {UI_WIDGET_TEXT,      nullptr,                                    nullptr},
    {UI_WIDGET_TEXT_EDIT, nullptr,                                    nullptr},
};
// clang-format on

void UITemplateSchemaSaver::save_widget_subtree(uint32_t idx)
{
    LD_ASSERT(mWriter.is_array_table_scope());

    const UITemplateEntry* entry = mTmpl->entries[idx];

    mWriter.begin_table();

    std::string typeStr(get_ui_widget_type_cstr(entry->type));
    mWriter.key("type").value_string(typeStr);
    mWriter.key("index").value_u32(idx);

    // save root widget in a single entry
    save_widget_layout(entry->layout);
    sUITemplateSchemaTable[(int)entry->type].save_toml(*this, *entry);

    for (uint32_t childIdx : mTmpl->hierarchy[idx])
    {
        save_widget_subtree(childIdx);
    }

    mWriter.end_table();
}

void UITemplateSchemaSaver::save_widget_layout(const UILayoutInfo& layout)
{
    mWriter.begin_inline_table("layout");

    switch (layout.sizeX.type)
    {
    case UI_SIZE_FIXED:
        mWriter.key("size_x").value_i32((int32_t)layout.sizeX.extent);
        break;
    case UI_SIZE_GROW:
        mWriter.key("size_x").value_string("grow");
        break;
    case UI_SIZE_WRAP:
        mWriter.key("size_x").value_string("wrap");
        break;
    case UI_SIZE_FIT:
        mWriter.key("size_x").value_string("fit");
        break;
    }

    switch (layout.sizeY.type)
    {
    case UI_SIZE_FIXED:
        mWriter.key("size_y").value_i32((int32_t)layout.sizeY.extent);
        break;
    case UI_SIZE_GROW:
        mWriter.key("size_y").value_string("grow");
        break;
    case UI_SIZE_WRAP:
        mWriter.key("size_y").value_string("wrap");
        break;
    case UI_SIZE_FIT:
        mWriter.key("size_y").value_string("fit");
        break;
    }

    switch (layout.childAlignX)
    {
    case UI_ALIGN_BEGIN:
        mWriter.key("child_align_x").value_string("begin");
        break;
    case UI_ALIGN_CENTER:
        mWriter.key("child_align_x").value_string("center");
        break;
    case UI_ALIGN_END:
        mWriter.key("child_align_x").value_string("end");
        break;
    }

    switch (layout.childAlignY)
    {
    case UI_ALIGN_BEGIN:
        mWriter.key("child_align_y").value_string("begin");
        break;
    case UI_ALIGN_CENTER:
        mWriter.key("child_align_y").value_string("center");
        break;
    case UI_ALIGN_END:
        mWriter.key("child_align_y").value_string("end");
        break;
    }

    switch (layout.childAxis)
    {
    case UI_AXIS_X:
        mWriter.key("child_axis").value_string("x");
        break;
    case UI_AXIS_Y:
        mWriter.key("child_axis").value_string("y");
        break;
    }

    mWriter.begin_inline_table("child_padding");
    mWriter.key("left").value_f32(layout.childPadding.left);
    mWriter.key("right").value_f32(layout.childPadding.right);
    mWriter.key("top").value_f32(layout.childPadding.top);
    mWriter.key("bottom").value_f32(layout.childPadding.bottom);
    mWriter.key("child_gap").value_f32(layout.childGap);

    mWriter.end_inline_table();
    mWriter.end_inline_table();
}

UITemplateSchemaSaver::~UITemplateSchemaSaver()
{
    if (mWriter)
        TOMLWriter::destroy(mWriter);
}

bool UITemplateSchemaSaver::save_template(UITemplateObj* obj, std::string& toml, std::string& err)
{
    mTmpl = obj;

    mWriter = TOMLWriter::create();
    mWriter.begin();

    mWriter.begin_table("ludens_ui_template");
    mWriter.key("versionMajor").value_u32(LD_VERSION_MAJOR);
    mWriter.key("versionMinor").value_u32(LD_VERSION_MINOR);
    mWriter.key("versionPatch").value_u32(LD_VERSION_PATCH);
    mWriter.end_table();

    mWriter.begin_array_table("widget");
    if (!mTmpl->entries.empty())
        save_widget_subtree(0);
    mWriter.end_array_table();

    mWriter.begin_table("hierarchy");
    for (auto it : mTmpl->hierarchy)
    {
        uint32_t parentIdx = it.first;
        mWriter.key(std::to_string(parentIdx)).begin_array();
        for (uint32_t childIdx : it.second)
            mWriter.value_u32(childIdx);
        mWriter.end_array();
    }
    mWriter.end_table();

    mWriter.end(toml);
    TOMLWriter::destroy(mWriter);

    return true;
}

void UITemplateSchemaSaver::save_ui_panel(UITemplateSchemaSaver& saver, const UITemplateEntry& entry)
{
    LD_ASSERT(saver.mWriter && saver.mWriter.is_table_scope());
    LD_ASSERT(entry.type == UI_WIDGET_TEXT);

    TOMLWriter writer = saver.mWriter;
    writer.key("color").value_u32((uint32_t)entry.panel.info.color);
}

void UITemplateSchemaSaver::save_ui_image(UITemplateSchemaSaver& saver, const UITemplateEntry& entry)
{
    LD_ASSERT(saver.mWriter && saver.mWriter.is_table_scope());
    LD_ASSERT(entry.type == UI_WIDGET_IMAGE);

    TOMLWriter writer = saver.mWriter;
    writer.key("texture_2d").value_u32((uint32_t)entry.image.texture2DAUID);

    writer.begin_inline_table("image_rect");
    TOMLUtil::save_rect_table(entry.image.imageRect, writer);
    writer.end_inline_table();
}

UITemplateSchemaLoader::~UITemplateSchemaLoader()
{
    if (mDoc)
        TOMLDocument::destroy(mDoc);
}

bool UITemplateSchemaLoader::load_template(UITemplateObj* obj, const View& toml, std::string& err)
{
    mTmpl = obj;
    mTmpl->reset();

    mDoc = TOMLDocument::create();
    if (!TOMLParser::parse(mDoc, toml, err))
        return false;

    TOMLValue sceneTOML = mDoc.get("ludens_ui_template");
    if (!sceneTOML || sceneTOML.type() != TOML_TYPE_TABLE)
        return false;

    int32_t version;
    TOMLValue versionTOML = sceneTOML["version_major"];
    if (!versionTOML || !versionTOML.get_i32(version) || version != LD_VERSION_MAJOR)
        return false;

    versionTOML = sceneTOML["version_minor"];
    if (!versionTOML || !versionTOML.get_i32(version) || version != LD_VERSION_MINOR)
        return false;

    versionTOML = sceneTOML["version_patch"];
    if (!versionTOML || !versionTOML.get_i32(version) || version != LD_VERSION_PATCH)
        return false;

    TOMLValue hierarchyTOML = mDoc.get("hierarchy");
    if (!hierarchyTOML || !hierarchyTOML.is_table())
        return false;

    std::vector<std::string> keys;
    hierarchyTOML.get_keys(keys);
    for (const std::string& key : keys)
    {
        uint32_t parentIdx = static_cast<uint32_t>(std::stoul(key));
        TOMLValue childrenTOML = hierarchyTOML[key.c_str()];
        if (!childrenTOML || !childrenTOML.is_array())
            continue;

        int count = childrenTOML.size();
        for (int i = 0; i < count; i++)
        {
            uint32_t childIdx;
            if (!childrenTOML[i].get_u32(childIdx))
                continue;

            mTmpl->hierarchy[parentIdx].push_back(childIdx);
        }
    }

    TOMLValue widgetArrayTOML = mDoc.get("widget");
    if (!widgetArrayTOML || widgetArrayTOML.type() != TOML_TYPE_ARRAY)
        return false;

    int widgetCount = widgetArrayTOML.size();
    mTmpl->entries.resize((size_t)widgetCount);

    for (int i = 0; i < widgetCount; i++)
    {
        TOMLValue widgetTOML = widgetArrayTOML[i];
        if (!widgetTOML || !widgetTOML.is_table())
            continue;

        load_widget_toml(widgetTOML);
    }

    return true;
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

    TOMLValue childAlignTOML = layoutTOML.get_key("child_align_x");
    if (!childAlignTOML || !load_layout_child_align_toml(layout.childAlignX, childAlignTOML))
        return false;

    childAlignTOML = layoutTOML.get_key("child_align_y");
    if (!childAlignTOML || !load_layout_child_align_toml(layout.childAlignY, childAlignTOML))
        return false;

    std::string str;
    TOMLValue childAxisTOML = layoutTOML.get_key("child_axis");
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

    TOMLValue childGapTOML = layoutTOML.get_key("child_gap", TOML_TYPE_FLOAT);
    if (childGapTOML)
        childGapTOML.get_f32(layout.childGap);

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

//
// Public API
//

bool UITemplateSchema::load_ui_template_from_source(UITemplate tmpl, const View& toml, std::string& err)
{
    LD_PROFILE_SCOPE;

    UITemplateSchemaLoader loader;
    if (!loader.load_template(tmpl.unwrap(), toml, err))
        return false;

    return true;
}

bool UITemplateSchema::load_ui_template_from_file(UITemplate tmpl, const FS::Path& tomlPath, std::string& err)
{
    LD_PROFILE_SCOPE;

    Vector<byte> toml;
    if (!FS::read_file_to_vector(tomlPath, toml))
        return false;

    View tomlView((const char*)toml.data(), toml.size());
    return load_ui_template_from_source(tmpl, tomlView, err);
}

bool UITemplateSchema::save_ui_template(UITemplate tmpl, const FS::Path& savePath, std::string& err)
{
    LD_PROFILE_SCOPE;

    UITemplateSchemaSaver saver;

    std::string toml;
    if (!saver.save_template(tmpl, toml, err))
        return false;

    return FS::write_file_and_swap_backup(savePath, toml.size(), (const byte*)toml.data(), err);
}

} // namespace LD