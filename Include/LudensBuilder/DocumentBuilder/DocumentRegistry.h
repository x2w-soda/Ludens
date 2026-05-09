#pragma once

#include <Ludens/DSA/HashMap.h>
#include <Ludens/Header/Handle.h>
#include <LudensBuilder/DocumentBuilder/Document.h>

namespace LD {

struct DocumentRegistryValidator
{
    bool (*onMiscURI)(View uri);
    String missingURI;
};

struct DocumentRegistry : Handle<struct DocumentRegistryObj>
{
    /// @brief Create empty document registry.
    static DocumentRegistry create();

    /// @brief Destroy registry and all documents within.
    static void destroy(DocumentRegistry registry);

    /// @brief Try add a document.
    bool add_document(const DocumentInfo& info, String& err);

    /// @brief Retrieve document from its URI path.
    Document get_document(const char* uriPath);

    /// @brief Validate if all URIs in documents are valid.
    bool validate(DocumentRegistryValidator& validator);
};

} // namespace LD