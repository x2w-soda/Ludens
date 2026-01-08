#pragma once

#include <Ludens/Asset/Template/UITemplate.h>
#include <Ludens/Header/View.h>
#include <Ludens/System/FileSystem.h>
#include <string>

namespace LD {

struct UITemplateSchema
{
    /// @brief Load a UI template from TOML schema source string.
    static bool load_ui_template_from_source(UITemplate tmpl, const View& toml, std::string& err);

    /// @brief Load a UI template from TOML schema file on disk.
    static bool load_ui_template_from_file(UITemplate tmpl, const FS::Path& tomlPath, std::string& err);

    /// @brief Try saving UI template as TOML schema file on disk.
    static bool save_ui_template(UITemplate tmpl, const FS::Path& savePath, std::string& err);
};

} // namespace LD