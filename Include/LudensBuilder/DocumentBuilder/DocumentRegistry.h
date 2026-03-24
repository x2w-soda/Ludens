#pragma once

#include <Ludens/DSA/HashMap.h>
#include <Ludens/Header/Handle.h>
#include <LudensBuilder/DocumentBuilder/Document.h>

namespace LD {

struct DocumentRegistry : Handle<struct DocumentRegistryObj>
{
    /// @brief Create empty document registry.
    static DocumentRegistry create();

    /// @brief Destroy registry and all documents within.
    static void destroy(DocumentRegistry registry);

    /// @brief Try add a document.
    bool add_document(const DocumentInfo& info, std::string& err);

    /// @brief Retrieve document from its URI.
    Document get_document(const char* uri);
};

} // namespace LD