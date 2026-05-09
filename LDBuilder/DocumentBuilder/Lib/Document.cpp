#include <Ludens/DSA/HashSet.h>
#include <Ludens/DSA/Stack.h>
#include <Ludens/DSA/Vector.h>
#include <Ludens/DSA/ViewUtil.h>
#include <Ludens/Header/Assert.h>
#include <Ludens/Media/Format/MD.h>
#include <Ludens/Memory/Allocator.h>
#include <Ludens/Memory/Memory.h>
#include <Ludens/Profiler/Profiler.h>
#include <LudensBuilder/DocumentBuilder/Document.h>
#include <LudensBuilder/DocumentBuilder/DocumentURI.h>

#include <format>
#include <string>

#define DOCUMENT_LA_PAGE_SIZE 1024

namespace LD {

static_assert(LD::IsTrivial<DocumentSpan>);
static_assert(LD::IsTrivial<DocumentSpanText>);
static_assert(LD::IsTrivial<DocumentSpanImage>);

static_assert(LD::IsTrivial<DocumentItem>);
static_assert(LD::IsTrivial<DocumentItemHeading>);
static_assert(LD::IsTrivial<DocumentItemParagraph>);
static_assert(LD::IsTrivial<DocumentItemCodeBlock>);
static_assert(LD::IsTrivial<DocumentItemListEntry>);

struct DocumentParseState
{
    DocumentItem* item;
};

struct DocumentObj
{
    String uriPath = {};
    DocumentRefs refs = {};
    Vector<char> copy;
    Vector<String*> strings;
    Vector<DocumentItem*> items;
    Vector<DocumentSpan*> spans;
    Stack<DocumentItem*> parseItems;
    Stack<DocumentSpan*> parseSpans;
    HashSet<String> parseURIs;
    LinearAllocator la;
    int spanCounter = 0;
    int listEntryIndex = 0;

    DocumentObj();
    DocumentObj(const DocumentObj&) = delete;
    ~DocumentObj();

    DocumentObj& operator=(const DocumentObj&) = delete;

    DocumentItem* allocate_item(DocumentItemType type);
    DocumentSpan* allocate_span(DocumentSpanType type);
    void add_uri(View uri);
    void pop_item();
    void pop_string_span();

    static int on_parser_enter_block(MDBlockType type, const MDBlockDetail& detail, void* user);
    static int on_parser_leave_block(MDBlockType type, const MDBlockDetail& detail, void* user);
    static int on_parser_enter_span(MDSpanType type, const MDSpanDetail& detail, void* user);
    static int on_parser_leave_span(MDSpanType type, const MDSpanDetail& detail, void* user);
    static int on_parser_text(MDTextType type, View text, void* user);
};

struct DocumentItemMeta
{
    DocumentItemType type;
    size_t byteSize;
    const char* typeCstr;
    String (*print)(DocumentItem* item);
};

static String print_document_item_heading(DocumentItem* item);
static String print_document_item_paragraph(DocumentItem* item);
static String print_document_item_code_block(DocumentItem* item);
static String print_document_item_list_entry(DocumentItem* item);

// clang-format off
static DocumentItemMeta sItemMeta[] = {
    {DOCUMENT_ITEM_HEADING,    sizeof(DocumentItemHeading),   "DocumentItemHeading",   &print_document_item_heading},
    {DOCUMENT_ITEM_PARAGRAPH,  sizeof(DocumentItemParagraph), "DocumentItemParagraph", &print_document_item_paragraph},
    {DOCUMENT_ITEM_CODE_BLOCK, sizeof(DocumentItemCodeBlock), "DocumentItemCodeBlock", &print_document_item_code_block},
    {DOCUMENT_ITEM_LIST_ENTRY, sizeof(DocumentItemListEntry), "DocumentItemListEntry", &print_document_item_list_entry},
};
// clang-format on

static_assert(sizeof(sItemMeta) / sizeof(*sItemMeta) == (int)DOCUMENT_ITEM_ENUM_COUNT);

static String print_document_item_heading(DocumentItem* item)
{
    auto* heading = (DocumentItemHeading*)item;
    LD_ASSERT(heading->item.spans.size > 0);

    DocumentSpan* span = heading->item.spans.data[0];
    String str = sItemMeta[item->type].typeCstr;
    std::string fmt = std::format(" level {}: {}\n", heading->level, span->text);
    str.append((const char8_t*)fmt.data(), fmt.size());

    return str;
}

static String print_document_item_paragraph(DocumentItem* item)
{
    String str = sItemMeta[item->type].typeCstr;
    std::string fmt = std::format(" {} spans\n", item->spans.size);
    str.append((const char8_t*)fmt.data(), fmt.size());

    return str;
}

static String print_document_item_code_block(DocumentItem* item)
{
    String str = sItemMeta[item->type].typeCstr;
    std::string fmt = std::format(" {} spans\n", item->spans.size);
    str.append((const char8_t*)fmt.data(), fmt.size());

    return str;
}

static String print_document_item_list_entry(DocumentItem* item)
{
    auto* entry = (DocumentItemListEntry*)item;

    String str = sItemMeta[item->type].typeCstr;
    std::string fmt = std::format(" {}\n", entry->index);
    str.append((const char8_t*)fmt.data(), fmt.size());

    return str;
}

DocumentObj::DocumentObj()
{
    LinearAllocatorInfo laI{};
    laI.isMultiPage = true;
    laI.capacity = DOCUMENT_LA_PAGE_SIZE;
    laI.usage = MEMORY_USAGE_DOCUMENT;
    la = LinearAllocator::create(laI);
}

DocumentObj::~DocumentObj()
{
    for (String* str : strings)
        heap_delete<String>(str);

    LinearAllocator::destroy(la);
}

DocumentItem* DocumentObj::allocate_item(DocumentItemType type)
{
    DocumentItem* item = (DocumentItem*)la.allocate(sItemMeta[(int)type].byteSize);
    LD_ASSERT(item);

    item->type = type;
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
    case DOCUMENT_SPAN_CODE:
        span = (DocumentSpan*)la.allocate(sizeof(DocumentSpanCode));
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

    spanCounter++;

    return span;
}

void DocumentObj::add_uri(View view)
{
    if (!view)
        return;

    URI uriView(view);
    String uriPath = document_uri_normalized_path(uriView);

    if (parseURIs.contains(uriPath))
        return;

    parseURIs.insert(uriPath);

    if (document_uri_is_manual(uriView))
        refs.manual.push_back(view);
    else if (document_uri_is_lua_api(uriView))
        refs.luaAPI.push_back(view);
    else
        refs.misc.push_back(view);
}

void DocumentObj::pop_item()
{
    DocumentItem* item = parseItems.top();
    parseItems.pop();

    item->spans.data = nullptr; // resolved later
    item->spans.size = (size_t)spanCounter;

    spanCounter = 0;
}

void DocumentObj::pop_string_span()
{
    DocumentSpan* span = parseSpans.top();
    String* code = strings.back();

    span->text = View((const byte*)code->data(), code->size());
    parseSpans.pop();
}

int DocumentObj::on_parser_enter_block(MDBlockType type, const MDBlockDetail& detail, void* user)
{
    auto* obj = (DocumentObj*)user;
    obj->spanCounter = 0;

    switch (type)
    {
    case MD_BLOCK_TYPE_DOC:
        break;
    case MD_BLOCK_TYPE_UL:
    {
        obj->listEntryIndex = -1;
        break;
    }
    case MD_BLOCK_TYPE_OL:
    {
        obj->listEntryIndex = detail.ol.start;
        break;
    }
    case MD_BLOCK_TYPE_LI:
    {
        auto* entry = (DocumentItemListEntry*)obj->allocate_item(DOCUMENT_ITEM_LIST_ENTRY);
        entry->index = -1;
        if (obj->listEntryIndex >= 0)
            entry->index = obj->listEntryIndex++;
        break;
    }
    case MD_BLOCK_TYPE_H:
    {
        auto* heading = (DocumentItemHeading*)obj->allocate_item(DOCUMENT_ITEM_HEADING);
        heading->level = detail.h.level;
        break;
    }
    case MD_BLOCK_TYPE_CODE:
    {
        auto* block = (DocumentItemCodeBlock*)obj->allocate_item(DOCUMENT_ITEM_CODE_BLOCK);
        block->lang = detail.code.lang;
        obj->strings.push_back(heap_new<String>(MEMORY_USAGE_DOCUMENT));
        (void)obj->allocate_span(DOCUMENT_SPAN_TEXT); // accumulate within code block
        break;
    }
    case MD_BLOCK_TYPE_P:
    {
        (void)obj->allocate_item(DOCUMENT_ITEM_PARAGRAPH);
        break;
    }
    default:
        LD_UNREACHABLE;
    }

    return 0;
}

int DocumentObj::on_parser_leave_block(MDBlockType type, const MDBlockDetail&, void* user)
{
    auto* obj = (DocumentObj*)user;

    switch (type)
    {
    case MD_BLOCK_TYPE_DOC:
        LD_ASSERT(obj->parseItems.empty());
        break;
    case MD_BLOCK_TYPE_CODE:
    {
        LD_ASSERT(obj->spanCounter == 1);
        obj->pop_string_span();
        obj->pop_item();
        break;
    }
    case MD_BLOCK_TYPE_H:
    case MD_BLOCK_TYPE_P:
    case MD_BLOCK_TYPE_LI:
        obj->pop_item();
        break;
    case MD_BLOCK_TYPE_UL:
    case MD_BLOCK_TYPE_OL:
        break;
    default:
        LD_DEBUG_BREAK;
        return 1;
    }

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
        obj->add_uri(link->href);
        break;
    }
    case MD_SPAN_TYPE_IMG:
    {
        auto* image = (DocumentSpanImage*)obj->allocate_span(DOCUMENT_SPAN_IMAGE);
        image->uri = detail.img.src;
        obj->add_uri(image->uri);
        break;
    }
    case MD_SPAN_TYPE_CODE:
        (void)obj->allocate_span(DOCUMENT_SPAN_CODE);
        break;
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
    obj->parseSpans.pop();

    switch (type)
    {
    case MD_SPAN_TYPE_A:
        LD_ASSERT(span->type == DOCUMENT_SPAN_LINK);
        break;
    case MD_SPAN_TYPE_IMG:
        LD_ASSERT(span->type == DOCUMENT_SPAN_IMAGE);
        break;
    case MD_SPAN_TYPE_CODE:
        LD_ASSERT(span->type == DOCUMENT_SPAN_CODE);
        break;
    default:
        LD_DEBUG_BREAK;
        return 1;
    }

    return 0;
}

int DocumentObj::on_parser_text(MDTextType type, View text, void* user)
{
    auto* obj = (DocumentObj*)user;
    LD_ASSERT(!obj->parseItems.empty());

    switch (type)
    {
    case MD_TEXT_TYPE_SOFT_BR:
        return 0; // soft line break '\n' ignored here
    case MD_TEXT_TYPE_CODE:
        if (obj->parseItems.top()->type == DOCUMENT_ITEM_CODE_BLOCK)
        {
            obj->strings.back()->append((const char8_t*)text.data, text.size);
            return 0;
        }
        break; // inline <code></code> span
    case MD_TEXT_TYPE_NORMAL:
        break;
    default:
        LD_DEBUG_BREAK;
        return 1;
    }

    DocumentSpan* span = nullptr;

    if (obj->parseSpans.empty()) // create span on the fly
    {
        span = obj->allocate_span(DOCUMENT_SPAN_TEXT);
        obj->parseSpans.pop();
    }
    else
        span = obj->parseSpans.top();

    span->text = text;

    return 0;
}

Document Document::create(const DocumentInfo& info, String& err)
{
    LD_PROFILE_SCOPE;

    auto* obj = heap_new<DocumentObj>(MEMORY_USAGE_DOCUMENT);

    obj->uriPath = info.uriPath;

    View view = info.md;
    if (info.copyData)
    {
        obj->copy.resize(info.md.size);
        std::copy(info.md.data, info.md.data + info.md.size, obj->copy.data());
        view = View((const byte*)obj->copy.data(), obj->copy.size());
    }

    const MDCallback callbacks = {
        .onEnterBlock = &DocumentObj::on_parser_enter_block,
        .onLeaveBlock = &DocumentObj::on_parser_leave_block,
        .onEnterSpan = &DocumentObj::on_parser_enter_span,
        .onLeaveSpan = &DocumentObj::on_parser_leave_span,
        .onText = &DocumentObj::on_parser_text,
    };

    if (!MDParser::parse(view, err, callbacks, obj))
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

    LD_ASSERT(obj->parseItems.empty());
    LD_ASSERT(obj->parseSpans.empty());
    obj->parseURIs.clear();

    return Document(obj);
}

void Document::destroy(Document doc)
{
    LD_PROFILE_SCOPE;

    auto* obj = doc.unwrap();

    heap_delete<DocumentObj>(obj);
}

String Document::get_uri_path()
{
    return mObj->uriPath;
}

DocumentRefs Document::get_references()
{
    return mObj->refs;
}

TView<DocumentItem*> Document::get_items()
{
    return TView<DocumentItem*>(mObj->items.data(), mObj->items.size());
}

String Document::print()
{
    String str;

    for (DocumentItem* item : mObj->items)
    {
        str += sItemMeta[item->type].print(item);
    }

    return str;
}

} // namespace LD
