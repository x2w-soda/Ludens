#pragma once

#include <Ludens/DSA/StringView.h>
#include <Ludens/Header/Handle.h>
#include <filesystem>
#include <vector>

namespace LD {

using XMLString = StringView;

struct XMLAttribute : Handle<struct XMLNode>
{
    /// @brief get next sibling attribute
    XMLAttribute get_next();

    /// @brief get attribute name
    XMLString get_name();

    /// @brief get attribute value
    XMLString get_value();
};

struct XMLElement : Handle<struct XMLNode>
{
    /// @brief get element name
    XMLString get_name();

    /// @brief get first attribute of element
    XMLAttribute get_attributes();

    /// @brief get first child element
    XMLElement get_child(XMLString& mixed);

    /// @brief get next sibling element
    XMLElement get_next(XMLString& mixed);
};

struct XMLDocument : Handle<struct XMLDocumentObj>
{
    static XMLDocument create();
    static XMLDocument create_from_file(const std::filesystem::path& path);
    static void destroy(XMLDocument doc);

    /// @brief parse XML source string
    /// @return true on success
    /// @warning all nodes from previous document are invalidated
    bool parse(const char* xml, size_t size);

    /// @brief get the attributes of the DOM declaration element
    XMLAttribute get_declaration();

    /// @brief get DOM root element
    XMLElement get_root();
};

} // namespace LD