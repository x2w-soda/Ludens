#pragma once

#include "Core/DSA/Include/View.h"
#include "Core/DSA/Include/Stack.h"
#include "Core/DSA/Include/Vector.h"
#include "Core/OS/Include/Allocator.h"
#include "Core/OS/Include/Memory.h"

namespace LD {

class XMLElement;
class XMLAttribute;
class XMLParser;
class XMLDocument;
struct XMLNode;

using XMLString = View<char>;

enum class XMLType
{
    Text,
    Element,
    Attribute,
};

class XMLText
{
    friend class XMLElement;

public:
    XMLText();
    XMLText(const XMLText&) = delete;
    ~XMLText();

    inline XMLString GetText()
    {
        return mText;
    }

    inline void SetText(const XMLString& text)
    {
        mText = text;
    }

    XMLText& operator=(const XMLText&) = delete;

private:
    XMLString mText;
};

class XMLElement
{
    friend class XMLParser;

public:
    XMLElement();
    XMLElement(const XMLElement&) = delete;
    ~XMLElement();

    XMLElement& operator=(const XMLElement&) = delete;

    /// get parent node
    XMLNode* GetParent();

    /// get next sibling node 
    XMLNode* GetNext();
    
    /// get first child node 
    XMLNode* GetChild();

    inline XMLString GetName()
    {
        return mName;
    };

    inline XMLAttribute* GetAttributes()
    {
        return mFirstAttribute;
    }

    void SetName(const XMLString& name);

    inline bool HasName(const char* match)
    {
        // TODO: normalize escape characters in name,
        //       operator== does not consider "&amp" vs "&"

        return mName == match;
    }

    /// append an attribute to this element
    XMLAttribute* AddAttribute(const XMLString& name, const XMLString& value);

    /// append a child text node
    XMLText* AddText(const XMLString& text);

    /// append a child element node 
    XMLElement* AddElement(const XMLString& name);

private:
    XMLString mName;
    XMLAttribute* mFirstAttribute = nullptr;
    XMLAttribute* mLastAttribute = nullptr;
};

class XMLAttribute
{
    friend class XMLElement;
    friend class XMLParser;

public:
    XMLAttribute();
    XMLAttribute(const XMLAttribute&) = delete;
    ~XMLAttribute();

    XMLAttribute& operator=(const XMLAttribute&) = delete;

    inline XMLAttribute* GetNext()
    {
        return mNext;
    }

    inline XMLString GetName()
    {
        return mName;
    }

    inline XMLString GetValue()
    {
        return mValue;
    }

    inline bool HasName(const char* match)
    {
        // TODO: normalize escape characters in name,
        //       operator== does not consider "&amp" vs "&"

        return mName == match;
    }

    void SetName(const XMLString& name);
    void SetValue(const XMLString& value);

private:
    char mQuote;        // single or double quotes surrounding the attribute value
    XMLString mName;
    XMLString mValue;
    XMLAttribute* mNext = nullptr;
};

struct XMLParserConfig
{
    /// if set to true, skips xml header parsing
    bool SkipHeader = false;
};

struct XMLTag
{
    XMLElement* Element;
    int Line;
};

class XMLParser
{
public:
    XMLParser() = delete;
    XMLParser(const XMLParserConfig& config);
    XMLParser(const XMLParser&) = delete;
    ~XMLParser();

    XMLParser& operator=(const XMLParser&) = delete;

    Ref<XMLDocument> ParseString(const char* xml, size_t size);

private:
    
    /// @brief try and parse a single element and its contents
    /// @return true if an element is parsed, false otherwise
    bool ParseElement(XMLDocument& doc);
    void ParseElementContent(XMLDocument& doc);

    /// @brief try and parse the xml header, storing result to mHeader
    /// @return true if the header is valid and parsed, false otherwise
    bool ParseHeader(XMLDocument& doc); 

    /// @brief try and parse a single tag
    /// @return 0 if a closing tag is parsed, 1 if a self closing tag is parsed, 2 if an opening tag is parsed
    int ParseTag(XMLDocument& doc);

    /// @brief parse attributes in an opening tag or self closing tag
    /// @return number of parsed attributes
    int ParseAttributes(XMLDocument& doc, XMLAttribute** frist, XMLAttribute** last);

    Stack<XMLTag> mTagStack;
    XMLParserConfig mConfig;
    XMLElement* mElement;
    const char* mXML;
    size_t mXMLSize;
    int mCursor;
};

class XMLNode
{
    friend class XMLElement;
    friend class XMLDocument;

public:

    /// get underyling type of the node
    inline XMLType GetType() const
    {
        return mType;
    }

    /// safe cast to text node, or nullptr 
    inline XMLText* ToText()
    {
        return mType == XMLType::Text ? (XMLText*)this : nullptr;
    }

    /// safe cast to element node, or nullptr 
    inline XMLElement* ToElement()
    {
        return mType == XMLType::Element ? (XMLElement*)this : nullptr;
    }

    /// safe cast to attribute node, or nullptr 
    inline XMLAttribute* ToAttribute()
    {
        return mType == XMLType::Attribute ? (XMLAttribute*)this : nullptr;
    }

    /// get next sibling node, or nullptr 
    inline XMLNode* GetNext()
    {
        return mNext;
    }

private:
    void AppendChild(XMLNode* child);

    union
    {
        XMLText uText;
        XMLElement uElement;
        XMLAttribute uAttribute;
    };
    XMLType mType;

    XMLDocument* mDocument;
    XMLNode* mParent;
    XMLNode* mNext;
    XMLNode* mFirstChild;
    XMLNode* mLastChild;
};

class XMLDocument
{
    friend class XMLElement;
    friend class XMLAttribute;
    friend class XMLParser;

public:
    XMLDocument();
    XMLDocument(const XMLDocument&) = delete;
    ~XMLDocument();

    XMLDocument& operator=(const XMLDocument&) = delete;

    inline XMLNode* GetChild()
    {
        return mFirstChild;
    }

    inline XMLElement* GetHeader()
    {
        return mHeader;
    }

    /// add a child element to the document
    XMLElement* AddElement(const XMLString& name);

private:
    using NodeAllocator = PoolAllocator<sizeof(XMLNode)>;

    XMLNode* AllocNode(XMLType type);
    void FreeNode(XMLNode* node);

    Vector<NodeAllocator> mPages;
    XMLElement* mHeader = nullptr;
    XMLNode* mFirstChild = nullptr;
    XMLNode* mLastChild = nullptr;
};

} // namespace LD