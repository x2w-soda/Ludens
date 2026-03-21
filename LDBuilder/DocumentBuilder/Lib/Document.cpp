#include <Ludens/DSA/Stack.h>
#include <Ludens/DSA/Vector.h>
#include <Ludens/Header/Assert.h>
#include <Ludens/Media/Format/MD.h>
#include <Ludens/Memory/Allocator.h>
#include <Ludens/Memory/Memory.h>
#include <Ludens/Profiler/Profiler.h>
#include <LudensBuilder/DocumentBuilder/Document.h>

#include <format>

#define DOCUMENT_MEMORY_USAGE MEMORY_USAGE_MISC
#define DOCUMENT_LA_PAGE_SIZE 1024

namespace LD {

struct DocumentParseState
{
    DocumentItem* item;
};

struct DocumentObj
{
    Vector<DocumentItem*> items;
    Vector<DocumentSpan*> spans;
    Stack<DocumentItem*> parseItems;
    LinearAllocator la;
    size_t spanCounter = 0;

    DocumentObj();
    DocumentObj(const DocumentObj&) = delete;
    ~DocumentObj();

    DocumentObj& operator=(const DocumentObj&) = delete;

    DocumentItem* allocate_item(DocumentItemType type);
    DocumentSpan* allocate_span(DocumentSpanType type);

    static int on_parser_enter_block(MDBlockType type, const MDBlockDetail& detail, void* user);
    static int on_parser_leave_block(MDBlockType type, const MDBlockDetail& detail, void* user);
    static int on_parser_enter_span(MDSpanType type, const MDSpanDetail& detail, void* user);
    static int on_parser_leave_span(MDSpanType type, const MDSpanDetail& detail, void* user);
    static int on_parser_text(MDTextType type, const View& text, void* user);
};

struct DocumentItemMeta
{
    DocumentItemType type;
    const char* typeCstr;
    std::string (*print)(DocumentItem* item);
};

static std::string print_document_item_heading(DocumentItem* item);
static std::string print_document_item_paragraph(DocumentItem* item);

// clang-format off
static DocumentItemMeta sItemMeta[] = {
    {DOCUMENT_ITEM_HEADING,   "DocumentItemHeading",   &print_document_item_heading},
    {DOCUMENT_ITEM_PARAGRAPH, "DocumentItemParagraph", &print_document_item_paragraph},
};
// clang-format on

static std::string print_document_item_heading(DocumentItem* item)
{
    auto* heading = (DocumentItemHeading*)item;
    LD_ASSERT(heading->item.spans.size > 0);

    DocumentSpan* span = heading->item.spans.data[0];
    std::string str = sItemMeta[item->type].typeCstr;
    str += std::format(" level {}: {}\n", heading->level, span->text);

    return str;
}

std::string print_document_item_paragraph(DocumentItem* item)
{
    std::string str = sItemMeta[item->type].typeCstr;
    str += std::format(" {} spans\n", item->spans.size);

    return str;
}

DocumentObj::DocumentObj()
{
    LinearAllocatorInfo laI{};
    laI.isMultiPage = true;
    laI.capacity = DOCUMENT_LA_PAGE_SIZE;
    laI.usage = DOCUMENT_MEMORY_USAGE;
    la = LinearAllocator::create(laI);
}

DocumentObj::~DocumentObj()
{
    LinearAllocator::destroy(la);
}

DocumentItem* DocumentObj::allocate_item(DocumentItemType type)
{
    DocumentItem* item = nullptr;

    switch (type)
    {
    case DOCUMENT_ITEM_HEADING:
        item = (DocumentItem*)la.allocate(sizeof(DocumentItemHeading));
        item->type = type;
        break;
    case DOCUMENT_ITEM_PARAGRAPH:
        item = (DocumentItem*)la.allocate(sizeof(DocumentItemParagraph));
        item->type = type;
        break;
    default:
        break;
    }

    LD_ASSERT(item);
    return item;
}

DocumentSpan* DocumentObj::allocate_span(DocumentSpanType type)
{
    DocumentSpan* span = nullptr;

    switch (type)
    {
    case DOCUMENT_SPAN_TEXT:
        span = (DocumentSpan*)la.allocate(sizeof(DocumentSpanText));
        span->type = type;
        break;
    default:
        break;
    }

    LD_ASSERT(span);
    span->text = {};
    return span;
}

int DocumentObj::on_parser_enter_block(MDBlockType type, const MDBlockDetail& detail, void* user)
{
    auto* obj = (DocumentObj*)user;

    DocumentItem* item = nullptr;

    switch (type)
    {
    case MD_BLOCK_TYPE_DOC:
        break;
    case MD_BLOCK_TYPE_H:
    {
        item = obj->allocate_item(DOCUMENT_ITEM_HEADING);
        obj->parseItems.push(item);
        auto* heading = (DocumentItemHeading*)item;
        heading->level = detail.h.level;
        break;
    }
    case MD_BLOCK_TYPE_P:
    {
        item = obj->allocate_item(DOCUMENT_ITEM_PARAGRAPH);
        obj->parseItems.push(item);
        break;
    }
    default:
        LD_UNREACHABLE;
    }

    obj->spanCounter;

    return 0;
}

int DocumentObj::on_parser_leave_block(MDBlockType type, const MDBlockDetail& detail, void* user)
{
    auto* obj = (DocumentObj*)user;

    if (type == MD_BLOCK_TYPE_DOC)
    {
        LD_ASSERT(obj->parseItems.empty());
        return 0;
    }

    DocumentItem* item = obj->parseItems.top();
    item->spans.data = nullptr; // resolved later
    item->spans.size = obj->spanCounter;

    obj->parseItems.pop();
    obj->spanCounter = 0;
    obj->items.push_back(item);

    return 0;
}

int DocumentObj::on_parser_enter_span(MDSpanType type, const MDSpanDetail& detail, void* user)
{
    auto* obj = (DocumentObj*)user;

    return 0;
}

int DocumentObj::on_parser_leave_span(MDSpanType type, const MDSpanDetail& detail, void* user)
{
    auto* obj = (DocumentObj*)user;

    return 0;
}

int DocumentObj::on_parser_text(MDTextType type, const View& text, void* user)
{
    auto* obj = (DocumentObj*)user;

    LD_ASSERT(type == MD_TEXT_TYPE_NORMAL);
    LD_ASSERT(!obj->parseItems.empty());

    DocumentSpan* span = obj->allocate_span(DOCUMENT_SPAN_TEXT);
    span->text = text;

    obj->spans.push_back(span);
    obj->spanCounter++;

    return 0;
}

Document Document::create(const View& md)
{
    LD_PROFILE_SCOPE;

    auto* obj = heap_new<DocumentObj>(DOCUMENT_MEMORY_USAGE);

    const MDCallback callbacks = {
        .onEnterBlock = &DocumentObj::on_parser_enter_block,
        .onLeaveBlock = &DocumentObj::on_parser_leave_block,
        .onEnterSpan = &DocumentObj::on_parser_enter_span,
        .onLeaveSpan = &DocumentObj::on_parser_leave_span,
        .onText = &DocumentObj::on_parser_text,
    };

    std::string err;
    if (!MDParser::parse(md, err, callbacks, obj))
    {
        heap_delete<DocumentObj>(obj);
        return {};
    }

    // vector sizes are fixed now, store spans
    size_t spanBase = 0;
    for (DocumentItem* item : obj->items)
    {
        item->spans.data = obj->spans.data() + spanBase;
        spanBase += item->spans.size;
    }

    return Document(obj);
}

void Document::destroy(Document doc)
{
    LD_PROFILE_SCOPE;

    auto* obj = doc.unwrap();

    heap_delete<DocumentObj>(obj);
}

TView<DocumentItem*> Document::get_items()
{
    return TView<DocumentItem*>(mObj->items.data(), mObj->items.size());
}

std::string Document::print()
{
    std::string str;

    for (DocumentItem* item : mObj->items)
    {
        str += sItemMeta[item->type].print(item);
    }

    return str;
}

} // namespace LD
