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

static_assert(LD::IsTrivial<DocumentSpan>);
static_assert(LD::IsTrivial<DocumentSpanText>);
static_assert(LD::IsTrivial<DocumentSpanImage>);

static_assert(LD::IsTrivial<DocumentItem>);
static_assert(LD::IsTrivial<DocumentItemHeading>);
static_assert(LD::IsTrivial<DocumentItemParagraph>);

struct DocumentParseState
{
    DocumentItem* item;
};

struct DocumentObj
{
    Vector<DocumentItem*> items;
    Vector<DocumentSpan*> spans;
    Stack<DocumentItem*> parseItems;
    Stack<DocumentSpan*> parseSpans;
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
    parseItems.push(item);
    items.push_back(item);

    return item;
}

DocumentSpan* DocumentObj::allocate_span(DocumentSpanType type)
{
    DocumentSpan* span = nullptr;

    switch (type)
    {
    case DOCUMENT_SPAN_TEXT:
        span = (DocumentSpan*)la.allocate(sizeof(DocumentSpanText));
        break;
    case DOCUMENT_SPAN_LINK:
        span = (DocumentSpan*)la.allocate(sizeof(DocumentSpanLink));
        break;
    case DOCUMENT_SPAN_IMAGE:
        span = (DocumentSpan*)la.allocate(sizeof(DocumentSpanImage));
        break;
    default:
        LD_UNREACHABLE;
    }

    span->type = type;
    span->text = {};
    parseSpans.push(span);
    spans.push_back(span);

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
        auto* heading = (DocumentItemHeading*)item;
        heading->level = detail.h.level;
        break;
    }
    case MD_BLOCK_TYPE_P:
    {
        item = obj->allocate_item(DOCUMENT_ITEM_PARAGRAPH);
        break;
    }
    default:
        LD_UNREACHABLE;
    }

    obj->spanCounter = 0;

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

    return 0;
}

int DocumentObj::on_parser_enter_span(MDSpanType type, const MDSpanDetail& detail, void* user)
{
    auto* obj = (DocumentObj*)user;

    switch (type)
    {
    case MD_SPAN_TYPE_A:
    {
        auto* link = (DocumentSpanLink*)obj->allocate_span(DOCUMENT_SPAN_LINK);
        link->span.text = detail.a.title;
        link->href = detail.a.href;
        link->title = detail.a.title;
        break;
    }
    case MD_SPAN_TYPE_IMG:
    {
        auto* image = (DocumentSpanImage*)obj->allocate_span(DOCUMENT_SPAN_IMAGE);
        image->uri = detail.img.src;
        break;
    }
    default:
        LD_UNREACHABLE;
    }

    return 0;
}

int DocumentObj::on_parser_leave_span(MDSpanType type, const MDSpanDetail& detail, void* user)
{
    auto* obj = (DocumentObj*)user;

    LD_ASSERT(!obj->parseSpans.empty());
    DocumentSpan* span = obj->parseSpans.top();

    switch (type)
    {
    case MD_SPAN_TYPE_A:
        LD_ASSERT(span->type == DOCUMENT_SPAN_LINK);
        break;
    case MD_SPAN_TYPE_IMG:
        LD_ASSERT(span->type == DOCUMENT_SPAN_IMAGE);
        break;
    default:
        LD_UNREACHABLE;
    }

    obj->parseSpans.pop();

    return 0;
}

int DocumentObj::on_parser_text(MDTextType type, const View& text, void* user)
{
    auto* obj = (DocumentObj*)user;

    switch (type)
    {
    case MD_TEXT_TYPE_SOFT_BR:
        return 0; // soft line break '\n' ignored here
    case MD_TEXT_TYPE_NORMAL:
        break;
    default:
        LD_UNREACHABLE;
    }

    LD_ASSERT(!obj->parseItems.empty());

    DocumentSpan* span = nullptr;

    if (obj->parseSpans.empty())
    {
        span = obj->allocate_span(DOCUMENT_SPAN_TEXT);
        obj->parseSpans.pop();
    }
    else
        span = obj->parseSpans.top();

    span->text = text;

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

    // vector sizes are fixed now, get address inside vector
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
