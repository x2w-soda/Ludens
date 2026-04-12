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
    DOCUMENT_SPAN_CODE,
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

struct DocumentSpanCode
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
    DOCUMENT_ITEM_CODE_BLOCK,
    DOCUMENT_ITEM_LIST_ENTRY,
    DOCUMENT_ITEM_ENUM_COUNT,
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

struct DocumentItemCodeBlock
{
    DocumentItem item;
    View lang;
};

struct DocumentItemListEntry
{
    DocumentItem item;
    int index;
};

/// @brief All URI references that a Document contains.
struct DocumentRefs
{
    Vector<View> manual; // docs://Manual
    Vector<View> luaAPI; // docs://LuaAPI
    Vector<View> misc;   // other unknown URI
};

/// @brief Document creation info.
struct DocumentInfo
{
    View md;
    const char* uriPath = nullptr;
    bool copyData = false;
};

struct Document : Handle<struct DocumentObj>
{
    /// @brief Try create Document.
    static Document create(const DocumentInfo& info, std::string& err);

    /// @brief Destroy document.
    static void destroy(Document doc);

    /// @brief Get document URI path.
    std::string get_uri_path();

    /// @brief Get all URI references from this document.
    DocumentRefs get_references();

    /// @brief Get all items in this Document.
    TView<DocumentItem*> get_items();

    /// @brief Debug print to string.
    std::string print();
};

} // namespace LD