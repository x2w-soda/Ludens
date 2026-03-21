#pragma once

#include <Ludens/Header/Color.h>
#include <Ludens/Header/Handle.h>
#include <Ludens/Header/View.h>

#include <string>

namespace LD {

enum DocumentSpanType
{
    DOCUMENT_SPAN_TEXT,
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

struct Document : Handle<struct DocumentObj>
{
    static Document create(const View& md);
    static void destroy(Document doc);

    TView<DocumentItem*> get_items();

    std::string print();
};

} // namespace LD