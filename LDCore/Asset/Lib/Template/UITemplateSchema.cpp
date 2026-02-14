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

    static void load_ui_panel_toml(UITemplateSchemaLoader& loader, UITemplateEntry& entry);
    static void load_ui_image_toml(UITemplateSchemaLoader& loader, UITemplateEntry& entry);

private:
    void load_widget_toml();
    bool load_layout_toml(UILayoutInfo& layout);
    bool load_layout_size_toml(UISize& size, const char* key);
    bool load_layout_child_align_toml(UIAlign& align, const char* key);
    bool load_layout_child_padding_toml(UIPadding& padding, const char* key);

private:
    UITemplateObj* mTmpl = nullptr;
    TOMLReader mReader{};
};

// clang-format off
struct
{
    UIWidgetType type;
    void (*save_toml)(UITemplateSchemaSaver& saver, const UITemplateEntry& entry);
    void (*load_toml)(UITemplateSchemaLoader& loader, UITemplateEntry& entry);
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

    mWriter.begin_table(UI_TEMPLATE_SCHEMA_TABLE);
    mWriter.key(UI_TEMPLATE_SCHEMA_KEY_VERSION_MAJOR).value_u32(LD_VERSION_MAJOR);
    mWriter.key(UI_TEMPLATE_SCHEMA_KEY_VERSION_MINOR).value_u32(LD_VERSION_MINOR);
    mWriter.key(UI_TEMPLATE_SCHEMA_KEY_VERSION_PATCH).value_u32(LD_VERSION_PATCH);
    mWriter.end_table();

    mWriter.begin_array_table(SCENE_SCHEMA_TABLE_WIDGET);
    if (!mTmpl->entries.empty())
        save_widget_subtree(0);
    mWriter.end_array_table();

    mWriter.begin_table(SCENE_SCHEMA_TABLE_HIERARCHY);
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
    writer.key("texture_2d").value_u32((uint32_t)entry.image.texture2DAssetID);
    TOMLUtil::write_rect(writer, "image_rect" , entry.image.imageRect);
}

UITemplateSchemaLoader::~UITemplateSchemaLoader()
{
    if (mReader)
        TOMLReader::destroy(mReader);
}

bool UITemplateSchemaLoader::load_template(UITemplateObj* obj, const View& toml, std::string& err)
{
    mTmpl = obj;
    mTmpl->reset();

    mReader = TOMLReader::create(toml, err);
    if (!mReader)
        return false;

    if (!mReader.enter_table(UI_TEMPLATE_SCHEMA_TABLE))
        return false;

    uint32_t version;
    if (!mReader.read_u32(UI_TEMPLATE_SCHEMA_KEY_VERSION_MAJOR, version) || version != LD_VERSION_MAJOR)
        return false;

    if (!mReader.read_u32(UI_TEMPLATE_SCHEMA_KEY_VERSION_MINOR, version) || version != LD_VERSION_MINOR)
        return false;

    if (!mReader.read_u32(UI_TEMPLATE_SCHEMA_KEY_VERSION_PATCH, version) || version != LD_VERSION_PATCH)
        return false;

    if (mReader.enter_table(SCENE_SCHEMA_TABLE_HIERARCHY))
    {
        Vector<std::string> keys;
        mReader.get_keys(keys);

        for (const std::string& key : keys)
        {
            uint32_t parentIdx = static_cast<uint32_t>(std::stoul(key));

            int count = 0;
            if (!mReader.enter_array(key.c_str(), count))
                continue;

            for (int i = 0; i < count; i++)
            {
                uint32_t childIdx;
                if (mReader.read_u32(i, childIdx))
                    mTmpl->hierarchy[parentIdx].push_back(childIdx);
            }

            mReader.exit();
        }
        mReader.exit();
    }

    int widgetCount = 0;
    if (mReader.enter_array(SCENE_SCHEMA_TABLE_WIDGET, widgetCount))
    {
        mTmpl->entries.resize((size_t)widgetCount);

        for (int i = 0; i < widgetCount; i++)
        {
            if (!mReader.enter_table(i))
                continue;

            load_widget_toml();
            mReader.exit();
        }

        mReader.exit();
    }
    return true;
}

void UITemplateSchemaLoader::load_ui_panel_toml(UITemplateSchemaLoader& loader, UITemplateEntry& entry)
{
    LD_ASSERT(entry.type == UI_WIDGET_PANEL);

    TOMLReader reader = loader.mReader;
    uint32_t colorU32 = 0;
    reader.read_u32("color", colorU32);

    entry.panel.info.color = Color(colorU32);
}

void UITemplateSchemaLoader::load_ui_image_toml(UITemplateSchemaLoader& loader, UITemplateEntry& entry)
{
    LD_ASSERT(entry.type == UI_WIDGET_IMAGE);
    TOMLReader reader = loader.mReader;

    entry.image.imageRect = {};
    entry.image.info.rect = &entry.image.imageRect;
    entry.image.info.image = {};

    TOMLUtil::read_rect(reader, "image_rect", entry.image.imageRect);

    entry.image.texture2DAssetID = 0;
    reader.read_u32("texture_2d", entry.image.texture2DAssetID);
}

void UITemplateSchemaLoader::load_widget_toml()
{
    uint32_t entryIdx;
    if (!mReader.read_u32("index", entryIdx))
        return;

    if (entryIdx >= mTmpl->entries.size())
        return;

    UITemplateEntry* entry = (UITemplateEntry*)mTmpl->entryPA.allocate();
    mTmpl->entries[entryIdx] = entry;

    std::string typeStr;
    if (!mReader.read_string("type", typeStr))
        return;

    if (mReader.enter_table("layout"))
    {
        bool ok = load_layout_toml(entry->layout);
        LD_ASSERT(ok); // TODO: invalid layout
        mReader.exit();
    }

    if (!get_ui_widget_type_from_cstr(entry->type, typeStr.c_str()))
        return;

    sUITemplateSchemaTable[(int)entry->type].load_toml(*this, *entry);
}

bool UITemplateSchemaLoader::load_layout_toml(UILayoutInfo& layout)
{
    layout = {};

    if (!load_layout_size_toml(layout.sizeX, "size_x") ||
        !load_layout_size_toml(layout.sizeY, "size_y") ||
        !load_layout_child_align_toml(layout.childAlignX, "child_align_x") ||
        !load_layout_child_align_toml(layout.childAlignY, "child_align_y") ||
        !load_layout_child_padding_toml(layout.childPadding, "child_padding"))
        return false;

    std::string str;
    if (!mReader.read_string("child_axis", str))
        return false;

    if (str == "x")
        layout.childAxis = UI_AXIS_X;
    else if (str == "y")
        layout.childAxis = UI_AXIS_Y;
    else
        return false;

    mReader.read_f32("child_gap", layout.childGap);

    return true;
}

bool UITemplateSchemaLoader::load_layout_size_toml(UISize& size, const char* key)
{
    std::string str;
    float f32;
    int32_t i32;

    if (mReader.read_string(key, str))
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

    if (mReader.read_f32(key, f32))
    {
        size = UISize::fixed(f32);
        return true;
    }

    if (mReader.read_i32(key, i32))
    {
        size = UISize::fixed((float)i32);
        return true;
    }

    return false;
}

bool UITemplateSchemaLoader::load_layout_child_align_toml(UIAlign& align, const char* key)
{
    std::string str;

    if (!mReader.read_string(key, str))
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

bool UITemplateSchemaLoader::load_layout_child_padding_toml(UIPadding& padding, const char* key)
{
    if (!mReader.enter_table(key))
        return false;

    if (!mReader.read_f32("left", padding.left) ||
        !mReader.read_f32("right", padding.right) ||
        !mReader.read_f32("top", padding.top) ||
        !mReader.read_f32("bottom", padding.bottom))
    {
        mReader.exit();
        return false;
    }

    mReader.exit();
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
    if (!FS::read_file_to_vector(tomlPath, toml, err))
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

    View tomlView(toml.data(), toml.size());
    return FS::write_file_and_swap_backup(savePath, tomlView, err);
}

} // namespace LD