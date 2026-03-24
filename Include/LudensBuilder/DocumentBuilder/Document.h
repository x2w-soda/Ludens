#pragma once

#include <Ludens/DSA/URI.h>
#include <Ludens/DSA/Vector.h>
#include <Ludens/Header/Color.h>
#include <Ludens/Header/Handle.h>
#include <Ludens/Header/View.h>

#include <string>

namespace LD {

enum DocumentSpanType
{
    DOCUMENT_SPAN_TEXT,
    DOCUMENT_SPAN_LINK,
    DOCUMENT_SPAN_IMAGE,
};

struct DocumentSpan
{
    DocumentSpanType type;
    View text;
};

struct DocumentSpanText
{
    DocumentSpan span;
};

struct DocumentSpanLink
{
    DocumentSpan span;
    View href;
    View title;
};

struct DocumentSpanImage
{
    DocumentSpan span;
    View uri;
};

enum DocumentItemType
{
    DOCUMENT_ITEM_HEADING,
    DOCUMENT_ITEM_PARAGRAPH,
};

struct DocumentItem
{
    DocumentItemType type;
    TView<DocumentSpan*> spans;
};

struct DocumentItemHeading
{
    DocumentItem item;
    int level;
};

struct DocumentItemParagraph
{
    DocumentItem item;
};

/// @brief All URI references that a Document contains.
struct DocumentRefs
{
    Vector<View> manual; // docs://manual
    Vector<View> api;    // docs://api
    Vector<View> misc;   // other unknown URI
};

/// @brief Document creation info.
struct DocumentInfo
{
    View md;
    const char* uri;
};

struct Document : Handle<struct DocumentObj>
{
    /// @brief Try create Document.
    static Document create(const DocumentInfo& info, std::string& err);

    /// @brief Destroy document.
    static void destroy(Document doc);

    /// @brief Get document URI.
    View get_uri();

    /// @brief Get all URI references from this document.
    DocumentRefs get_references();

    /// @brief Get all items in this Document.
    TView<DocumentItem*> get_items();

    /// @brief Debug print to string.
    std::string print();
};

} // namespace LD