#pragma once

#include <Ludens/Asset/Template/UITemplate.h>

namespace LD {

struct UITemplateSchema
{
    /// @brief Load a UI template from TOML schema source string.
    static void load_ui_template_from_source(UITemplate tmpl, const void* source, size_t len);

    /// @brief Load a UI template from TOML schema file on disk.
    static void load_ui_template_from_file(UITemplate tmpl, const FS::Path& tomlPath);

    /// @brief Try saving UI template as TOML schema file on disk.
    static bool save_ui_template(UITemplate tmpl, const FS::Path& savePath, std::string& err);
};

} // namespace LD