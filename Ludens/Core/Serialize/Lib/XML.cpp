#include <cctype>
#include "Core/Serialize/Include/XML.h"

#define NUM_NODES_PER_PAGE 1024

namespace LD {

// NOTE: currently assumes ASCII string, one byte per character.
//       will implement UTF8 sooner or later.

static inline void EatWhiteSpace(const char* str, int* cursor)
{
    int c = *cursor;

    while (str[c] && isspace(str[c]))
        c++;

    *cursor = c;
}

static inline XMLString EatWord(const char* str, int* cursor, const char* delims = "\0")
{
    int beg = *cursor;
    int end = beg;

    LD_DEBUG_ASSERT(!isspace(str[beg]) && "word should not begin with space");

    while (str[end] && !isspace(str[end]) && !strchr(delims, str[end]))
        end++;

    size_t len = end - beg;
    *cursor = end;
    return {len, str + beg};
}

static inline XMLString EatUntil(const char* str, int* cursor, const char* delims)
{
    int beg = *cursor;
    int end = beg;

    while (str[end] && !strchr(delims, str[end]))
        end++;

    size_t len = end - beg;
    *cursor = end;
    return {len, str + beg};
}

static inline bool StringEqual(const XMLString& lhs, const XMLString& rhs)
{
    if (lhs.Size() != rhs.Size())
        return false;

    const char* l = lhs.Data();
    const char* r = rhs.Data();
    size_t len = lhs.Size();

    for (size_t i = 0; i < len; i++)
        if (l[i] != r[i])
            return false;

    return true;
}

static inline bool StringEqual(const XMLString& str, const char* cstr)
{
    size_t len = strlen(cstr);

    if (str.Size() != len)
        return false;

    const char* data = str.Data();

    for (size_t i = 0; i < len; i++)
        if (data[i] != cstr[i])
            return false;

    return true;
}

XMLText::XMLText()
{
}

XMLText::~XMLText()
{
}

XMLElement::XMLElement()
{
}

XMLElement::~XMLElement()
{
}

XMLNode* XMLElement::GetParent()
{
    return ((XMLNode*)this)->mParent;
}

XMLNode* XMLElement::GetNext()
{
    return ((XMLNode*)this)->mNext;
}

XMLNode* XMLElement::GetChild()
{
    return ((XMLNode*)this)->mFirstChild;
}

void XMLElement::SetName(const XMLString& name)
{
    mName = name;
}

XMLAttribute* XMLElement::AddAttribute(const XMLString& name, const XMLString& value)
{
    XMLNode* node = (XMLNode*)this;
    XMLDocument* doc = node->mDocument;
    XMLAttribute* attribute = (XMLAttribute*)doc->AllocNode(XMLType::Attribute);
    attribute->SetName(name);
    attribute->SetValue(value);

    if (mFirstAttribute == nullptr)
        mFirstAttribute = mLastAttribute = attribute;
    else
        mLastAttribute = mLastAttribute->mNext = attribute;

    return attribute;
}

XMLText* XMLElement::AddText(const XMLString& text)
{
    XMLNode* node = (XMLNode*)this;
    XMLDocument* doc = node->mDocument;
    XMLText* child = (XMLText*)doc->AllocNode(XMLType::Text);

    child->mText = text;

    node->AppendChild((XMLNode*)child);

    return child;
}

XMLElement* XMLElement::AddElement(const XMLString& name)
{
    XMLNode* node = (XMLNode*)this;
    XMLDocument* doc = node->mDocument;
    XMLElement* child = (XMLElement*)doc->AllocNode(XMLType::Element);

    child->mName = name;
    child->mFirstAttribute = nullptr;
    child->mLastAttribute = nullptr;

    node->AppendChild((XMLNode*)child);

    return child;
}

XMLAttribute::XMLAttribute() : mQuote('\0')
{
}

XMLAttribute::~XMLAttribute()
{
}

void XMLAttribute::SetName(const XMLString& name)
{
    mName = name;
}

void XMLAttribute::SetValue(const XMLString& value)
{
    mValue = value;
}

XMLParser::XMLParser(const XMLParserConfig& config) : mConfig(config)
{
    mTagStack.Clear();
    mElement = nullptr;
    mXML = nullptr;
    mCursor = 0;
}

XMLParser::~XMLParser()
{
}

Ref<XMLDocument> XMLParser::ParseString(const char* xml, size_t size)
{
    Ref<XMLDocument> doc = MakeRef<XMLDocument>();

    mXML = xml;
    mXMLSize = size;
    mCursor = 0;
    mElement = nullptr;

    if (!mConfig.SkipHeader)
    {
        bool validHeader = ParseHeader(*doc);

        if (!validHeader)
            return nullptr;
    }

    while (ParseElement(*doc))
        mElement = nullptr;

    LD_DEBUG_ASSERT(mTagStack.IsEmpty());

    return doc;
}

bool XMLParser::ParseElement(XMLDocument& doc)
{
    int tagType = ParseTag(doc);

    if (tagType == 2)
    {
        // found an opening tag, parse until we find its closing tag
        LD_DEBUG_ASSERT(!mTagStack.IsEmpty());
        size_t depth = mTagStack.Size() - 1;

        // parse mixed content, an element can contain any mixture of text and child elements
        while (mTagStack.Size() > depth)
            ParseElementContent(doc);

        return true;
    }

    return tagType == 1;
}

void XMLParser::ParseElementContent(XMLDocument& doc)
{
    int textBeg = mCursor;
    
    EatWhiteSpace(mXML, &mCursor);

    char c = mXML[mCursor];

    if (c == '<')
    {
        ParseTag(doc);
    }
    else // append as text
    {
        LD_DEBUG_ASSERT(mElement && "text node must be a child of some element node");

        int textEnd = textBeg;

        while (c && c != '<')
        {
            c = mXML[++textEnd];
        }

        XMLString text(textEnd - textBeg, mXML + textBeg);
        mElement->AddText(text);
        mCursor = textEnd;
    }
}

bool XMLParser::ParseHeader(XMLDocument& doc)
{
    EatWhiteSpace(mXML, &mCursor);

    if (mXML[mCursor] != '<' || mXML[mCursor + 1] != '?')
        return false;

    mCursor += 2;
    EatWhiteSpace(mXML, &mCursor);

    XMLString word = EatWord(mXML, &mCursor, "?");
    if (!StringEqual(word, "xml"))
        return false;

    // NOTE: Even if header parsing fails, these header attributes are not freed.
    //       All nodes will eventually be freed during ~XMLDocument without leakage.
    XMLAttribute* firstAttr;
    XMLAttribute* lastAttr;
    ParseAttributes(doc, &firstAttr, &lastAttr);

    if (mXML[mCursor] != '?' || mXML[mCursor + 1] != '>')
        return false;

    mCursor += 2;

    doc.mHeader = (XMLElement*)doc.AllocNode(XMLType::Element);
    doc.mHeader->mName = { 3, "xml" };
    doc.mHeader->mFirstAttribute = firstAttr;
    doc.mHeader->mLastAttribute = lastAttr;

    return true;
}

int XMLParser::ParseTag(XMLDocument& doc)
{
    EatWhiteSpace(mXML, &mCursor);

    if (mXML[mCursor] != '<')
        return false;

    ++mCursor;
    EatWhiteSpace(mXML, &mCursor);

    // closing tag, whose name should match with the top of tag stack
    if (mXML[mCursor] == '/')
    {
        ++mCursor;
        EatWhiteSpace(mXML, &mCursor);

        XMLString name = EatWord(mXML, &mCursor, ">");
        EatWhiteSpace(mXML, &mCursor);
        LD_DEBUG_ASSERT(mXML[mCursor] == '>' && "bad closing tag syntax");
        ++mCursor;

        XMLTag tag = mTagStack.Top();
        bool match = StringEqual(tag.Element->GetName(), name);
        LD_DEBUG_ASSERT(match && "opening and closing tags do not match");

        mTagStack.Pop();
        if (!mTagStack.IsEmpty())
            mElement = mTagStack.Top().Element;

        // closing tag
        return 0;
    }

    // Opening tag, or self closing tag.
    // Can contain attributes.
    XMLString name = EatWord(mXML, &mCursor, "/>");
    EatWhiteSpace(mXML, &mCursor);

    XMLAttribute* firstAttr;
    XMLAttribute* lastAttr;
    int numAttrs = ParseAttributes(doc, &firstAttr, &lastAttr);
    EatWhiteSpace(mXML, &mCursor);

    XMLElement* newElement = nullptr;
    int tagType;

    if (mXML[mCursor] == '>')
    {
        // handle opening tag
        ++mCursor;

        if (!mElement)
        {
            // top level element of document
            mElement = doc.AddElement(name);
        }
        else
        {
            // increase document tree depth
            mElement = mElement->AddElement(name);
        }

        newElement = mElement;

        XMLTag tag;
        tag.Element = newElement;
        mTagStack.Push(tag);

        // opening tag
        tagType = 2;
    }
    else if (mXML[mCursor] == '/')
    {
        // handle self closing tag
        ++mCursor;
        EatWhiteSpace(mXML, &mCursor);
        LD_DEBUG_ASSERT(mXML[mCursor] == '>');
        ++mCursor;

        if (!mElement)
        {
            // top level element of document
            newElement = doc.AddElement(name);
        }
        else
        {
            // append as child of top element
            newElement = mElement->AddElement(name);
        }

        // self closing tag
        tagType = 1;
    }
    else
        LD_DEBUG_UNREACHABLE;

    if (numAttrs > 0)
    {
        newElement->mFirstAttribute = firstAttr;
        newElement->mLastAttribute = lastAttr;
    }

    return tagType;
}

int XMLParser::ParseAttributes(XMLDocument& doc, XMLAttribute** first, XMLAttribute** last)
{
    XMLAttribute* head = nullptr;
    XMLAttribute* tail = nullptr;
    int numAttrs = 0;

    EatWhiteSpace(mXML, &mCursor);

    char c = mXML[mCursor];

    while (c && c != '>' && c != '/' && c != '?')
    {
        XMLString name = EatWord(mXML, &mCursor, "=");

        EatWhiteSpace(mXML, &mCursor);

        LD_DEBUG_ASSERT(mXML[mCursor] == '=');

        ++mCursor;
        EatWhiteSpace(mXML, &mCursor);
        char quote = mXML[mCursor++];

        LD_DEBUG_ASSERT(quote == '\'' || quote == '"');

        EatWhiteSpace(mXML, &mCursor);

        char delim[2] = {quote, '\0'};
        XMLString value = EatUntil(mXML, &mCursor, delim);

        LD_DEBUG_ASSERT(mXML[mCursor] == quote);
        ++mCursor;

        EatWhiteSpace(mXML, &mCursor);

        XMLAttribute* attr = (XMLAttribute*)doc.AllocNode(XMLType::Attribute);
        attr->mName = name;
        attr->mValue = value;
        attr->mQuote = quote;

        if (!head)
            head = tail = attr;
        else
            tail = tail->mNext = attr;

        ++numAttrs;

        c = mXML[mCursor];
    }

    *first = head;
    *last = tail;
    return numAttrs;
}

void XMLNode::AppendChild(XMLNode* child)
{
    child->mParent = this;

    if (!mFirstChild)
        mFirstChild = mLastChild = child;
    else
        mLastChild = mLastChild->mNext = child;
}

XMLDocument::XMLDocument()
{
    NodeAllocator allocator;
    allocator.Startup(NUM_NODES_PER_PAGE);
    mPages.PushBack(allocator);

    mFirstChild = nullptr;
    mLastChild = nullptr;
}

XMLDocument::~XMLDocument()
{
    for (NodeAllocator& page : mPages)
        page.Cleanup();
}

XMLElement* XMLDocument::AddElement(const XMLString& name)
{
    XMLNode* node = AllocNode(XMLType::Element);
    XMLElement* element = &node->uElement;
    element->SetName(name);

    if (mFirstChild == nullptr)
        mFirstChild = mLastChild = node;
    else
        mLastChild = mLastChild->mNext = node;

    return element;
}

XMLNode* XMLDocument::AllocNode(XMLType type)
{
    NodeAllocator* page = nullptr;

    for (size_t i = 0; i < mPages.Size(); i++)
    {
        if (mPages[i].CountFreeChunks() > 0)
        {
            page = mPages.Begin() + i;
            break;
        }
    }

    if (page == nullptr)
    {
        NodeAllocator newPage;
        newPage.Startup(NUM_NODES_PER_PAGE);
        mPages.PushBack(newPage);
        page = mPages.End() - 1;
    }

    XMLNode* node = (XMLNode*)page->Alloc(sizeof(XMLNode));
    memset(node, 0, sizeof(XMLNode));

    node->mType = type;
    node->mDocument = this;
    return node;
}

void XMLDocument::FreeNode(XMLNode* node)
{
    for (NodeAllocator& page : mPages)
    {
        if (page.Contains(node))
        {
            page.Free(node);
            return;
        }
    }
}

} // namespace LD