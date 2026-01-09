#include <Ludens/Header/Assert.h>
#include <Ludens/Media/Format/XML.h>
#include <Ludens/Memory/Allocator.h>
#include <Ludens/System/FileSystem.h>
#include <cctype>

namespace fs = std::filesystem;

namespace LD {

enum XMLTag
{
    XML_TAG_INVALID = 0,
    XML_TAG_OPENING,
    XML_TAG_CLOSING,
    XML_TAG_SELF_CLOSING,
};

struct XMLNode
{
    XMLNode* child;       /// first child
    XMLNode* next;        /// sibling linked list
    XMLNode* attr;        /// first attribute
    XMLString mixedChild; /// mixed content string before first child
    XMLString mixedNext;  /// mixed content string before next sibling
    XMLString name;       /// element or attribute name
    XMLString value;      /// attribute value
    bool isElement;       /// is element or attribute
};

/// @brief XML document implementation, each document
///        has its own parsing context.
struct XMLDocumentObj
{
    XMLNode* decl;
    XMLNode* root;
    PoolAllocator nodePA;
    const char* sourceXML;
    byte* fileBuffer;
    size_t sourceSize;
    size_t parsePos;

    XMLNode* alloc_node(bool isElement);

    bool parse_document(const char* buf, size_t size);
    XMLNode* parse_declaration();
    XMLNode* parse_element();
    XMLNode* parse_attributes();
    XMLTag parse_tag(XMLString* name, XMLNode** attrs);
    XMLString parse_string(const char* delims);

    inline bool consume(char c)
    {
        if (sourceXML[parsePos] != c)
            return false;

        parsePos++;
        return true;
    }

    inline bool match(const char* cstr)
    {
        for (size_t i = 0; (parsePos + i) < sourceSize && cstr[i]; i++)
        {
            if (sourceXML[parsePos + i] != cstr[i])
                return false;
        }

        return true;
    }

    inline void skip_whitespace()
    {
        while (parsePos < sourceSize && isspace(sourceXML[parsePos]))
            parsePos++;
    }

    inline XMLString trim_tail(const XMLString& XMLString)
    {
        const char* data = XMLString.data;
        size_t size = XMLString.size;

        while (size > 0 && isspace(data[size - 1]))
            size--;

        return View(data, size);
    }
};

XMLNode* XMLDocumentObj::alloc_node(bool isElement)
{
    XMLNode* e = (XMLNode*)nodePA.allocate();
    e->child = nullptr;
    e->next = nullptr;
    e->attr = nullptr;
    e->mixedChild = {};
    e->mixedNext = {};
    e->name = {};
    e->value = {};
    e->isElement = isElement;

    return e;
}

bool XMLDocumentObj::parse_document(const char* buf, size_t size)
{
    sourceXML = buf;
    sourceSize = size;
    parsePos = 0;
    decl = parse_declaration();
    root = parse_element();

    if (!root)
    {
        // TODO: error handling?
        return false;
    }

    return true;
}

XMLNode* XMLDocumentObj::parse_declaration()
{
    if (!match("<?xml"))
        return nullptr;
    parsePos += 5;

    XMLNode* declAttrs = parse_attributes();

    if (!match("?>"))
        return nullptr;
    parsePos += 2;

    return declAttrs;
}

XMLNode* XMLDocumentObj::parse_element()
{
    XMLString tagName{};
    XMLNode* attrs;
    size_t oldParsePos = parsePos;
    XMLTag tagType = parse_tag(&tagName, &attrs);

    if (tagType == XML_TAG_CLOSING)
    {
        // TODO: peek for "< /" instead of backtracking the whole tag (wasted attribute parsing)
        parsePos = oldParsePos;
        return {};
    }

    if (tagType == XML_TAG_INVALID)
        return {};

    XMLNode* element = alloc_node(true);
    element->name = tagName;
    element->attr = attrs;

    if (tagType == XML_TAG_OPENING)
    {
        element->mixedChild = parse_string("<");
        element->child = parse_element();

        for (XMLNode* lastChild = element->child; lastChild; lastChild = lastChild->next)
        {
            lastChild->mixedNext = parse_string("<");
            lastChild->next = parse_element();
        }

        XMLString closingTagName;
        XMLTag closingTagType = parse_tag(&closingTagName, &attrs);

        if (closingTagType != XML_TAG_CLOSING || tagName != closingTagName)
        {
            // TODO: error
            return {};
        }
    }

    return element;
}

XMLNode* XMLDocumentObj::parse_attributes()
{
    XMLNode dummy{.next = nullptr};
    XMLNode* lastAttr = &dummy;

    skip_whitespace();

    while (isalpha(sourceXML[parsePos]))
    {
        XMLString attrName = parse_string("=");
        attrName = trim_tail(attrName);

        parsePos++;
        skip_whitespace();

        if (!consume('\'') && !consume('"'))
        {
            // TODO: error invalid attribute value
            return nullptr;
        }

        XMLString attrValue = parse_string("'\"");
        parsePos++;
        skip_whitespace();

        lastAttr = lastAttr->next = alloc_node(false);
        lastAttr->name = attrName;
        lastAttr->value = attrValue;
    }

    return dummy.next;
}

XMLTag XMLDocumentObj::parse_tag(XMLString* name, XMLNode** attrs)
{
    *attrs = nullptr;

    skip_whitespace();

    char c = sourceXML[parsePos++];
    if (c != '<')
        return XML_TAG_INVALID;

    skip_whitespace();
    c = sourceXML[parsePos];

    // closing tag
    if (c == '/')
    {
        parsePos++;
        skip_whitespace();

        *name = parse_string(">");
        *name = trim_tail(*name);

        if (!consume('>'))
            return XML_TAG_INVALID;

        return XML_TAG_CLOSING;
    }

    // opening tag, or self-closing tag.
    // these may contain attributes
    *name = parse_string(" />");
    *name = trim_tail(*name);
    *attrs = parse_attributes();

    bool hasSlash = consume('/');
    skip_whitespace();

    if (consume('>'))
        return hasSlash ? XML_TAG_SELF_CLOSING : XML_TAG_OPENING;

    return XML_TAG_INVALID;
}

XMLString XMLDocumentObj::parse_string(const char* delims)
{
    const char* base = sourceXML + parsePos;
    size_t size = 0;

    while (parsePos < sourceSize)
    {
        char c = sourceXML[parsePos];
        bool foundDelim = false;

        for (const char* delim = delims; *delim; delim++)
        {
            if (c == *delim)
            {
                foundDelim = true;
                break;
            }
        }

        if (foundDelim)
            break;

        parsePos++;
        size++;
    }

    return XMLString(base, size);
}

#pragma region PublicAPI

XMLAttribute XMLAttribute::get_next()
{
    return {mObj->next};
}

XMLString XMLAttribute::get_name()
{
    return mObj->name;
}

XMLString XMLAttribute::get_value()
{
    return mObj->value;
}

XMLString XMLElement::get_name()
{
    return mObj->name;
}

XMLAttribute XMLElement::get_attributes()
{
    return {mObj->attr};
}

XMLElement XMLElement::get_child(XMLString& mixed)
{
    mixed = mObj->mixedChild;
    return {mObj->child};
}

XMLElement XMLElement::get_next(XMLString& mixed)
{
    mixed = mObj->mixedNext;
    return {mObj->next};
}

XMLDocument XMLDocument::create()
{
    XMLDocumentObj* obj = (XMLDocumentObj*)heap_malloc(sizeof(XMLDocumentObj), MEMORY_USAGE_MEDIA);
    obj->nodePA = {};
    obj->root = nullptr;
    obj->fileBuffer = nullptr;

    return {obj};
}

XMLDocument XMLDocument::create_from_file(const std::filesystem::path& path)
{
    if (!FS::exists(path))
        return {};

    XMLDocument doc = XMLDocument::create();
    XMLDocumentObj* obj = doc;

    std::string err; // TODO:
    uint64_t fileSize = FS::get_file_size(path);
    obj->fileBuffer = (byte*)heap_malloc(fileSize, MEMORY_USAGE_MEDIA);
    bool ok = FS::read_file(path, MutView((char*)obj->fileBuffer, fileSize), err);

    if (!ok)
    {
        XMLDocument::destroy(doc);
        return {};
    }

    doc.parse((const char*)obj->fileBuffer, fileSize);

    return {obj};
}

void XMLDocument::destroy(XMLDocument doc)
{
    XMLDocumentObj* obj = doc;

    if (obj->nodePA)
        PoolAllocator::destroy(obj->nodePA);

    if (obj->fileBuffer)
        heap_free(obj->fileBuffer);

    heap_free(obj);
}

bool XMLDocument::parse(const char* xml, size_t size)
{
    if (mObj->nodePA)
        PoolAllocator::destroy(mObj->nodePA);

    PoolAllocatorInfo paI{};
    paI.blockSize = sizeof(XMLNode);
    paI.isMultiPage = true;
    paI.pageSize = 64; // nodes per page
    paI.usage = MEMORY_USAGE_MEDIA;
    mObj->nodePA = PoolAllocator::create(paI);

    return mObj->parse_document(xml, size);
}

XMLAttribute XMLDocument::get_declaration()
{
    return {mObj->decl};
}

XMLElement XMLDocument::get_root()
{
    return {mObj->root};
}

void XMLParseJob::submit(const std::vector<std::filesystem::path>& paths)
{
    mHeader.type = 0;
    mHeader.user = this;
    mHeader.fn = &XMLParseJob::execute;
    mPaths = paths;

    JobSystem js = JobSystem::get();
    js.submit(&mHeader, JOB_DISPATCH_STANDARD);
}

void XMLParseJob::get_results(std::vector<XMLDocument>& docs)
{
    docs = mDocs;
}

void XMLParseJob::execute(void* user)
{
    XMLParseJob& self = *(XMLParseJob*)user;

    self.mDocs.resize(self.mPaths.size());

    for (size_t i = 0; i < self.mPaths.size(); i++)
    {
        self.mDocs[i] = XMLDocument::create_from_file(self.mPaths[i]);
    }
}

#pragma endregion PublicAPI

} // namespace LD