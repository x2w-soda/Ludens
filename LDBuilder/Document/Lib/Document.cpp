#include <Ludens/DSA/Stack.h>
#include <Ludens/DSA/Vector.h>
#include <Ludens/Header/Assert.h>
#include <Ludens/Media/Format/MD.h>
#include <Ludens/Memory/Allocator.h>
#include <Ludens/Memory/Memory.h>
#include <Ludens/Profiler/Profiler.h>
#include <LudensBuilder/Document/Document.h>

#include <format>

#define DOCUMENT_MEMORY_USAGE MEMORY_USAGE_MISC
#define DOCUMENT_LA_PAGE_SIZE 1024

namespace LD {

enum DocumentItemType
{
    DOCUMENT_ITEM_HEADING,
};

struct DocumentItem
{
    DocumentItemType type;
    size_t size;
};

struct DocumentItemHeading
{
    DocumentItem item;
    View text;
    int level;
};

struct DocumentParseState
{
    DocumentItem* item;
};

struct DocumentObj
{
    Vector<DocumentItem*> items;
    Stack<DocumentParseState> parseState;
    LinearAllocator itemLA;

    DocumentObj();
    DocumentObj(const DocumentObj&) = delete;
    ~DocumentObj();

    DocumentObj& operator=(const DocumentObj&) = delete;

    DocumentItem* push_state(DocumentItemType type);
    void pop_state(DocumentItemType type);

    static int on_parser_enter_block(MDBlockType type, const MDBlockDetail& detail, void* user);
    static int on_parser_leave_block(MDBlockType type, const MDBlockDetail& detail, void* user);
    static int on_parser_text(MDTextType type, const View& text, void* user);
};

struct DocumentItemMeta
{
    DocumentItemType type;
    const char* typeCstr;
    std::string (*print)(DocumentItem* item);
};

static std::string print_document_item_heading(DocumentItem* item);

// clang-format off
static DocumentItemMeta sItemMeta[] = {
    {DOCUMENT_ITEM_HEADING, "DocumentItemHeading", &print_document_item_heading}
};
// clang-format on

static std::string print_document_item_heading(DocumentItem* item)
{
    auto* heading = (DocumentItemHeading*)item;

    std::string str = sItemMeta[item->type].typeCstr;
    str += std::format(" level {}: {}\n", heading->level, heading->text);

    return str;
}

DocumentObj::DocumentObj()
{
    LinearAllocatorInfo laI{};
    laI.isMultiPage = true;
    laI.capacity = DOCUMENT_LA_PAGE_SIZE;
    laI.usage = DOCUMENT_MEMORY_USAGE;
    itemLA = LinearAllocator::create(laI);
}

DocumentObj::~DocumentObj()
{
    LinearAllocator::destroy(itemLA);
}

DocumentItem* DocumentObj::push_state(DocumentItemType type)
{
    DocumentItem* item = nullptr;

    switch (type)
    {
    case DOCUMENT_ITEM_HEADING:
        item = (DocumentItem*)itemLA.allocate(sizeof(DocumentItemHeading));
        item->type = type;
        item->size = sizeof(DocumentItemHeading);
        break;
    default:
        break;
    }

    LD_ASSERT(item);
    parseState.emplace(item);

    return item;
}

void DocumentObj::pop_state(DocumentItemType typeCheck)
{
    LD_ASSERT(parseState.top().item->type == typeCheck);

    parseState.pop();
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
        item = obj->push_state(DOCUMENT_ITEM_HEADING);
        obj->items.push_back(item);
        auto* heading = (DocumentItemHeading*)item;
        heading->level = detail.h.level;
        break;
    }
    default:
        LD_UNREACHABLE;
    }

    return 0;
}

int DocumentObj::on_parser_leave_block(MDBlockType type, const MDBlockDetail& detail, void* user)
{
    auto* obj = (DocumentObj*)user;

    switch (type)
    {
    case MD_BLOCK_TYPE_DOC:
        break;
    case MD_BLOCK_TYPE_H:
    {
        obj->pop_state(DOCUMENT_ITEM_HEADING);
        break;
    }
    default:
        LD_UNREACHABLE;
    }

    return 0;
}

int DocumentObj::on_parser_text(MDTextType type, const View& text, void* user)
{
    auto* obj = (DocumentObj*)user;

    LD_ASSERT(type == MD_TEXT_TYPE_NORMAL);

    DocumentItem* item = obj->parseState.top().item;

    switch (item->type)
    {
    case DOCUMENT_ITEM_HEADING:
    {
        DocumentItemHeading* heading = (DocumentItemHeading*)item;
        heading->text = text;
        break;
    }
    default:
        LD_UNREACHABLE;
    }

    return 0;
}

Document Document::create(const View& md)
{
    LD_PROFILE_SCOPE;

    auto* obj = heap_new<DocumentObj>(DOCUMENT_MEMORY_USAGE);

    const MDCallback callbacks = {
        .onEnterBlock = &DocumentObj::on_parser_enter_block,
        .onLeaveBlock = &DocumentObj::on_parser_leave_block,
        .onText = &DocumentObj::on_parser_text,
    };

    std::string err;
    if (!MDParser::parse(md, err, callbacks, obj))
    {
        heap_delete<DocumentObj>(obj);
        return {};
    }

    return Document(obj);
}

void Document::destroy(Document doc)
{
    LD_PROFILE_SCOPE;

    auto* obj = doc.unwrap();

    heap_delete<DocumentObj>(obj);
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
