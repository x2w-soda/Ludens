#include <Ludens/Asset/Template/UISchema.h>
#include <Ludens/Header/Assert.h>
#include <Ludens/Header/Version.h>
#include <Ludens/Media/Format/TOML.h>
#include <Ludens/Profiler/Profiler.h>

#include "UISchemaKeys.h"
#include "UITemplateObj.h"

namespace LD {

/// @brief Saves a UITemplate to TOML schema.
class UISchemaSaver
{
public:
    UISchemaSaver() = default;
    UISchemaSaver(const UISchemaSaver&) = delete;
    ~UISchemaSaver();

    UISchemaSaver& operator=(const UISchemaSaver&) = delete;

    /// @brief Save template as TOML schema.
    bool save_template(UITemplateObj* obj, std::string& toml, UISchema::Status& err);

    static void save_ui_button(UISchemaSaver& saver, const UITemplateEntry& entry);
    static void save_ui_panel(UISchemaSaver& saver, const UITemplateEntry& entry);
    static void save_ui_image(UISchemaSaver& saver, const UITemplateEntry& entry);

private:
    void save_widget_subtree(uint32_t idx);
    void save_widget_layout(const UILayoutInfo& layout);

private:
    UITemplateObj* mTmpl = nullptr;
    TOMLWriter mWriter{};
};

/// @brief Loads a UITemplate from TOML schema.
class UISchemaLoader
{
public:
    UISchemaLoader() = default;
    UISchemaLoader(const UISchemaLoader&) = delete;
    ~UISchemaLoader();

    UISchemaLoader& operator=(const UISchemaLoader&) = delete;

    bool load_template(UITemplateObj* obj, const View& toml, UISchema::Status& err);

    static void load_ui_button(UISchemaLoader& loader, UITemplateEntry& entry);
    static void load_ui_panel_toml(UISchemaLoader& loader, UITemplateEntry& entry);
    static void load_ui_image_toml(UISchemaLoader& loader, UITemplateEntry& entry);

private:
    bool load_widget_toml(UISchema::Status& err);
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
    void (*save_toml)(UISchemaSaver& saver, const UITemplateEntry& entry);
    void (*load_toml)(UISchemaLoader& loader, UITemplateEntry& entry);
} sUISchemaTable[] = {
    {UI_WIDGET_WINDOW,    nullptr,                                    nullptr},
    {UI_WIDGET_SCROLL,    nullptr,                                    nullptr},
    {UI_WIDGET_BUTTON,    &UISchemaSaver::save_ui_button,     &UISchemaLoader::load_ui_button},
    {UI_WIDGET_SLIDER,    nullptr,                                    nullptr},
    {UI_WIDGET_TOGGLE,    nullptr,                                    nullptr},
    {UI_WIDGET_PANEL,     &UISchemaSaver::save_ui_panel,      &UISchemaLoader::load_ui_panel_toml},
    {UI_WIDGET_IMAGE,     &UISchemaSaver::save_ui_image,      &UISchemaLoader::load_ui_image_toml},
    {UI_WIDGET_TEXT,      nullptr,                                    nullptr},
    {UI_WIDGET_TEXT_EDIT, nullptr,                                    nullptr},
};
// clang-format on

void UISchemaSaver::save_widget_subtree(uint32_t idx)
{
    LD_ASSERT(mWriter.is_array_table_scope());

    const UITemplateEntry* entry = mTmpl->entries[idx];

    mWriter.begin_table();

    std::string typeStr(get_ui_widget_type_cstr(entry->type));
    mWriter.key("type").write_string(typeStr);
    mWriter.key("name").write_string(entry->name);
    mWriter.key("index").write_u32(idx);

    // save root widget in a single entry
    save_widget_layout(entry->layout);
    sUISchemaTable[(int)entry->type].save_toml(*this, *entry);

    for (uint32_t childIdx : mTmpl->hierarchy[idx])
    {
        save_widget_subtree(childIdx);
    }

    mWriter.end_table();
}

void UISchemaSaver::save_widget_layout(const UILayoutInfo& layout)
{
    mWriter.begin_inline_table("layout");

    switch (layout.sizeX.type)
    {
    case UI_SIZE_FIXED:
        mWriter.key("size_x").write_i32((int32_t)layout.sizeX.extent);
        break;
    case UI_SIZE_GROW:
        mWriter.key("size_x").write_string("grow");
        break;
    case UI_SIZE_WRAP:
        mWriter.key("size_x").write_string("wrap");
        break;
    case UI_SIZE_FIT:
        mWriter.key("size_x").write_string("fit");
        break;
    }

    switch (layout.sizeY.type)
    {
    case UI_SIZE_FIXED:
        mWriter.key("size_y").write_i32((int32_t)layout.sizeY.extent);
        break;
    case UI_SIZE_GROW:
        mWriter.key("size_y").write_string("grow");
        break;
    case UI_SIZE_WRAP:
        mWriter.key("size_y").write_string("wrap");
        break;
    case UI_SIZE_FIT:
        mWriter.key("size_y").write_string("fit");
        break;
    }

    switch (layout.childAlignX)
    {
    case UI_ALIGN_BEGIN:
        mWriter.key("child_align_x").write_string("begin");
        break;
    case UI_ALIGN_CENTER:
        mWriter.key("child_align_x").write_string("center");
        break;
    case UI_ALIGN_END:
        mWriter.key("child_align_x").write_string("end");
        break;
    }

    switch (layout.childAlignY)
    {
    case UI_ALIGN_BEGIN:
        mWriter.key("child_align_y").write_string("begin");
        break;
    case UI_ALIGN_CENTER:
        mWriter.key("child_align_y").write_string("center");
        break;
    case UI_ALIGN_END:
        mWriter.key("child_align_y").write_string("end");
        break;
    }

    switch (layout.childAxis)
    {
    case UI_AXIS_X:
        mWriter.key("child_axis").write_string("x");
        break;
    case UI_AXIS_Y:
        mWriter.key("child_axis").write_string("y");
        break;
    }

    mWriter.begin_inline_table("child_padding");
    mWriter.key("left").write_f32(layout.childPadding.left);
    mWriter.key("right").write_f32(layout.childPadding.right);
    mWriter.key("top").write_f32(layout.childPadding.top);
    mWriter.key("bottom").write_f32(layout.childPadding.bottom);
    mWriter.key("child_gap").write_f32(layout.childGap);

    mWriter.end_inline_table();
    mWriter.end_inline_table();
}

UISchemaSaver::~UISchemaSaver()
{
    if (mWriter)
        TOMLWriter::destroy(mWriter);
}

bool UISchemaSaver::save_template(UITemplateObj* obj, std::string& toml, UISchema::Status& err)
{
    mTmpl = obj;

    mWriter = TOMLWriter::create();
    mWriter.begin();

    mWriter.begin_table(UI_TEMPLATE_SCHEMA_TABLE);
    mWriter.key(UI_TEMPLATE_SCHEMA_KEY_VERSION_MAJOR).write_u32(LD_VERSION_MAJOR);
    mWriter.key(UI_TEMPLATE_SCHEMA_KEY_VERSION_MINOR).write_u32(LD_VERSION_MINOR);
    mWriter.key(UI_TEMPLATE_SCHEMA_KEY_VERSION_PATCH).write_u32(LD_VERSION_PATCH);
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
            mWriter.write_u32(childIdx);
        mWriter.end_array();
    }
    mWriter.end_table();

    mWriter.end(toml);
    TOMLWriter::destroy(mWriter);

    return true;
}

void UISchemaSaver::save_ui_button(UISchemaSaver& saver, const UITemplateEntry& entry)
{
    LD_ASSERT(saver.mWriter && saver.mWriter.is_table_scope());
    LD_ASSERT(entry.type == UI_WIDGET_BUTTON);

    TOMLWriter writer = saver.mWriter;
    writer.key("text").write_string(entry.button.storage.text);
}

void UISchemaSaver::save_ui_panel(UISchemaSaver& saver, const UITemplateEntry& entry)
{
    LD_ASSERT(saver.mWriter && saver.mWriter.is_table_scope());
    LD_ASSERT(entry.type == UI_WIDGET_TEXT);

    TOMLWriter writer = saver.mWriter;
    writer.key("color").write_u32((uint32_t)entry.panel.storage.color);
}

void UISchemaSaver::save_ui_image(UISchemaSaver& saver, const UITemplateEntry& entry)
{
    LD_ASSERT(saver.mWriter && saver.mWriter.is_table_scope());
    LD_ASSERT(entry.type == UI_WIDGET_IMAGE);

    TOMLWriter writer = saver.mWriter;
    writer.key("texture_2d").write_u32((uint32_t)entry.image.texture2DAssetID);
    TOMLUtil::write_rect(writer, "image_rect", entry.image.storage.rect);
}

UISchemaLoader::~UISchemaLoader()
{
    if (mReader)
        TOMLReader::destroy(mReader);
}

bool UISchemaLoader::load_template(UITemplateObj* obj, const View& toml, UISchema::Status& err)
{
    mTmpl = obj;
    mTmpl->reset();

    mReader = TOMLReader::create(toml, err.str);
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

    mReader.exit();

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

    bool valid = true;

    int widgetCount = 0;
    if (mReader.enter_array(SCENE_SCHEMA_TABLE_WIDGET, widgetCount))
    {
        mTmpl->entries.resize((size_t)widgetCount);

        for (int i = 0; valid && i < widgetCount; i++)
        {
            if (!mReader.enter_table(i))
                continue;

            if (!load_widget_toml(err))
                valid = false;

            mReader.exit();
        }

        mReader.exit();
    }

    return valid;
}

void UISchemaLoader::load_ui_button(UISchemaLoader& loader, UITemplateEntry& entry)
{
    LD_ASSERT(entry.type == UI_WIDGET_BUTTON);

    TOMLReader reader = loader.mReader;

    entry.button.storage = {};
    reader.read_string("text", entry.button.storage.text);
}

void UISchemaLoader::load_ui_panel_toml(UISchemaLoader& loader, UITemplateEntry& entry)
{
    LD_ASSERT(entry.type == UI_WIDGET_PANEL);

    TOMLReader reader = loader.mReader;
    uint32_t colorU32 = 0;
    reader.read_u32("color", colorU32);

    entry.panel.storage = {};
    entry.panel.storage.color = Color(colorU32);
}

void UISchemaLoader::load_ui_image_toml(UISchemaLoader& loader, UITemplateEntry& entry)
{
    LD_ASSERT(entry.type == UI_WIDGET_IMAGE);
    TOMLReader reader = loader.mReader;

    entry.image.storage = {};
    TOMLUtil::read_rect(reader, "image_rect", entry.image.storage.rect);

    entry.image.texture2DAssetID = 0;
    reader.read_suid("texture_2d", entry.image.texture2DAssetID);
}

bool UISchemaLoader::load_widget_toml(UISchema::Status& err)
{
    uint32_t entryIdx;
    if (!mReader.read_u32("index", entryIdx))
        return false;

    if (entryIdx >= mTmpl->entries.size())
        return false;

    UIWidgetType type;
    std::string typeStr;
    if (!mReader.read_string("type", typeStr) || !get_ui_widget_type_from_cstr(type, typeStr.c_str()))
        return false;

    UITemplateEntry* entry = mTmpl->allocate_entry(type);
    mTmpl->entries[entryIdx] = entry;

    if (!mReader.read_string("name", entry->name))
        return false;

    if (mReader.enter_table("layout"))
    {
        bool ok = load_layout_toml(entry->layout);
        LD_ASSERT(ok); // TODO: invalid layout
        mReader.exit();
    }
    else
        entry->layout = {};

    sUISchemaTable[(int)entry->type].load_toml(*this, *entry);
    return true;
}

bool UISchemaLoader::load_layout_toml(UILayoutInfo& layout)
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

bool UISchemaLoader::load_layout_size_toml(UISize& size, const char* key)
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

bool UISchemaLoader::load_layout_child_align_toml(UIAlign& align, const char* key)
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

bool UISchemaLoader::load_layout_child_padding_toml(UIPadding& padding, const char* key)
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

bool UISchema::load_ui_template_from_source(UITemplate tmpl, const View& toml, Status& err)
{
    LD_PROFILE_SCOPE;

    UISchemaLoader loader;
    if (!loader.load_template(tmpl.unwrap(), toml, err))
        return false;

    return true;
}

bool UISchema::load_ui_template_from_file(UITemplate tmpl, const FS::Path& tomlPath, Status& err)
{
    LD_PROFILE_SCOPE;

    Vector<byte> toml;
    if (!FS::read_file_to_vector(tomlPath, toml, err.str))
        return false;

    View tomlView((const char*)toml.data(), toml.size());
    return load_ui_template_from_source(tmpl, tomlView, err);
}

bool UISchema::save_ui_template(UITemplate tmpl, const FS::Path& savePath, Status& err)
{
    LD_PROFILE_SCOPE;

    UISchemaSaver saver;

    std::string toml;
    if (!saver.save_template(tmpl, toml, err))
        return false;

    View tomlView(toml.data(), toml.size());
    return FS::write_file_and_swap_backup(savePath, tomlView, err.str);
}

} // namespace LD