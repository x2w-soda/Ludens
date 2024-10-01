#pragma once

#include <doctest.h>
#include "Core/Serialize/Include/XML.h"

using namespace LD;

static bool Equal(const XMLString& xml, const char* str)
{
    size_t len = strlen(str);
    if (xml.Size() != len)
        return false;

    const char* data = xml.Data();
    for (size_t i = 0; i < len; i++)
    {
        if (data[i] != str[i])
            return false;
    }

    return true;
}

TEST_CASE("XML Header")
{
    XMLParserConfig config{};
    XMLParser parser(config);

    {
        auto fuzz = [](XMLParser& parser, const char* xml) {
            Ref<XMLDocument> doc = parser.ParseString(xml, strlen(xml));
            CHECK(doc);

            XMLElement* header = doc->GetHeader();
            CHECK(header);
            CHECK(Equal(header->GetName(), "xml"));

            XMLAttribute* attr = header->GetAttributes();
            CHECK(!attr);
        };

        fuzz(parser, "<?xml?>");
        fuzz(parser, "<?xml  ?>");
    }

    {
        auto fuzz = [](XMLParser& parser, const char* xml) {
            Ref<XMLDocument> doc = parser.ParseString(xml, strlen(xml));
            CHECK(doc);

            XMLElement* header = doc->GetHeader();
            CHECK(header);
            CHECK(Equal(header->GetName(), "xml"));

            XMLAttribute* attr = header->GetAttributes();
            CHECK(attr);
            CHECK(Equal(attr->GetName(), "version"));
            CHECK(Equal(attr->GetValue(), "1.0"));

            attr = attr->GetNext();
            CHECK(attr);
            CHECK(Equal(attr->GetName(), "encoding"));
            CHECK(Equal(attr->GetValue(), "UTF-8"));

            attr = attr->GetNext();
            CHECK(attr);
            CHECK(Equal(attr->GetName(), "standalone"));
            CHECK(Equal(attr->GetValue(), "no"));

            attr = attr->GetNext();
            CHECK(!attr);
        };

        fuzz(parser, "<?xml version='1.0' encoding='UTF-8' standalone='no'?>");
        fuzz(parser, "<?xml version = '1.0' encoding = 'UTF-8' standalone = 'no' ?>");
    }
}

TEST_CASE("XML Single Element")
{
    XMLParserConfig config{};
    config.SkipHeader = true;

    XMLParser parser(config);

    // single element, no content
    {
        auto fuzz = [](XMLParser& parser, const char* xml) {
            Ref<XMLDocument> doc = parser.ParseString(xml, strlen(xml));

            CHECK(doc);

            XMLNode* node = doc->GetChild();
            CHECK(node);
            CHECK(node->GetType() == XMLType::Element);
            XMLElement* element = node->ToElement();
            XMLString name = element->GetName();
            CHECK(Equal(name, "h1"));

            CHECK(!element->GetChild());

            doc = nullptr;
        };

        fuzz(parser, "<h1></h1>");
        fuzz(parser, "< h1 >< /h1 >");
        fuzz(parser, "<h1>  </ h1>");
        fuzz(parser, "<h1/>");
        fuzz(parser, "<h1/ >");
        fuzz(parser, "<h1 />");
    }

    // single element, pure text content
    {
        auto fuzz = [](XMLParser& parser, const char* xml) {
            Ref<XMLDocument> doc = parser.ParseString(xml, strlen(xml));

            CHECK(doc);

            XMLNode* node = doc->GetChild();
            CHECK(node);
            CHECK(node->GetType() == XMLType::Element);

            XMLElement* element = node->ToElement();
            XMLString name = element->GetName();
            CHECK(Equal(name, "tag"));

            node = element->GetChild();
            CHECK(node);
            CHECK(node->GetType() == XMLType::Text);

            XMLText* text = node->ToText();
            CHECK(Equal(text->GetText(), "content"));

            doc = nullptr;
        };

        fuzz(parser, "<tag>content</tag>");
        fuzz(parser, "<tag >content< /tag>");
        fuzz(parser, "< tag>content</tag >");
    }

    // single element, attributes
    {
        auto fuzz = [](XMLParser& parser, const char* xml) {
            Ref<XMLDocument> doc = parser.ParseString(xml, strlen(xml));

            CHECK(doc);

            XMLNode* node = doc->GetChild();
            CHECK(node);
            CHECK(node->GetType() == XMLType::Element);

            XMLElement* element = node->ToElement();
            CHECK(Equal(element->GetName(), "h1"));

            XMLAttribute* attr = element->GetAttributes();
            CHECK(attr);
            CHECK(Equal(attr->GetName(), "name"));
            CHECK(Equal(attr->GetValue(), "value"));

            doc = nullptr;
        };

        fuzz(parser, "<h1 name='value'></h1>");
        fuzz(parser, "<h1 name=\"value\"></h1>");
        fuzz(parser, "<h1 name = 'value'></h1>");
        fuzz(parser, "<h1 name ='value' ></h1>");
        fuzz(parser, "<h1 name='value'/>");
    }
}

TEST_CASE("XML Top Level Elements")
{
    XMLParserConfig config{};
    config.SkipHeader = true;

    XMLParser parser(config);

    // multiple top level elements
    {
        auto fuzz = [](XMLParser& parser, const char* xml) {
            Ref<XMLDocument> doc = parser.ParseString(xml, strlen(xml));
            CHECK(doc);

            XMLNode* node = doc->GetChild();
            CHECK(node);
            CHECK(node->GetType() == XMLType::Element);

            XMLElement* memberdef = node->ToElement();
            CHECK(Equal(memberdef->GetName(), "memberdef"));

            {
                XMLAttribute* kind = memberdef->GetAttributes();
                CHECK(kind);
                CHECK(Equal(kind->GetName(), "kind"));
                CHECK(Equal(kind->GetValue(), "function"));

                XMLAttribute* id = kind->GetNext();
                CHECK(id);
                CHECK(Equal(id->GetName(), "id"));
                CHECK(Equal(id->GetValue(), "1"));

                XMLAttribute* prot = id->GetNext();
                CHECK(prot);
                CHECK(Equal(prot->GetName(), "prot"));
                CHECK(Equal(prot->GetValue(), "public"));
            }

            memberdef = memberdef->GetNext()->ToElement();
            CHECK(Equal(memberdef->GetName(), "memberdef"));

            {
                XMLAttribute* kind = memberdef->GetAttributes();
                CHECK(kind);
                CHECK(Equal(kind->GetName(), "kind"));
                CHECK(Equal(kind->GetValue(), "function"));

                XMLAttribute* id = kind->GetNext();
                CHECK(id);
                CHECK(Equal(id->GetName(), "id"));
                CHECK(Equal(id->GetValue(), "2"));

                XMLAttribute* prot = id->GetNext();
                CHECK(prot);
                CHECK(Equal(prot->GetName(), "prot"));
                CHECK(Equal(prot->GetValue(), "private"));
            }

            memberdef = memberdef->GetNext()->ToElement();
            CHECK(Equal(memberdef->GetName(), "memberdef"));

            {
                XMLAttribute* kind = memberdef->GetAttributes();
                CHECK(kind);
                CHECK(Equal(kind->GetName(), "kind"));
                CHECK(Equal(kind->GetValue(), "function"));

                XMLAttribute* id = kind->GetNext();
                CHECK(id);
                CHECK(Equal(id->GetName(), "id"));
                CHECK(Equal(id->GetValue(), "3"));

                XMLAttribute* prot = id->GetNext();
                CHECK(prot);
                CHECK(Equal(prot->GetName(), "prot"));
                CHECK(Equal(prot->GetValue(), "private"));

                XMLAttribute* inlin = prot->GetNext();
                CHECK(inlin);
                CHECK(Equal(inlin->GetName(), "inline"));
                CHECK(Equal(inlin->GetValue(), "yes"));
            }
            doc = nullptr;
        };

        fuzz(parser, "<memberdef kind='function' id='1' prot='public'></memberdef>\n"
                     "<memberdef kind='function' id='2' prot='private'></memberdef>\n"
                     "<memberdef kind='function' id='3' prot='private' inline='yes'></memberdef>");

        fuzz(parser, "<memberdef kind='function' id='1' prot='public'/>\n"
                     "<memberdef kind='function' id='2' prot='private'/>\n"
                     "< memberdef kind = 'function' id = '3' prot = 'private' inline = 'yes' > < / memberdef >");
    }
}

TEST_CASE("XML Nested Elements")
{
    XMLParserConfig config{};
    config.SkipHeader = true;

    XMLParser parser(config);

    {
        const char* xml = "<p><b><i>bold and italic</i></b></p>";
        Ref<XMLDocument> doc = parser.ParseString(xml, strlen(xml));
        CHECK(doc);

        XMLElement* e = doc->GetChild()->ToElement();
        CHECK(e);
        CHECK(Equal(e->GetName(), "p"));

        e = e->GetChild()->ToElement();
        CHECK(e);
        CHECK(Equal(e->GetName(), "b"));

        e = e->GetChild()->ToElement();
        CHECK(e);
        CHECK(Equal(e->GetName(), "i"));

        XMLText* text = e->GetChild()->ToText();
        CHECK(text);
        CHECK(Equal(text->GetText(), "bold and italic"));

        doc = nullptr;
    }

    {
        const char* xml = "<table rows='3' border-radius='10px'><tr>row 1</tr><tr>row 2</tr><tr>row 3</tr></table>";
        Ref<XMLDocument> doc = parser.ParseString(xml, strlen(xml));
        CHECK(doc);

        XMLElement* table = doc->GetChild()->ToElement();
        CHECK(table);
        CHECK(Equal(table->GetName(), "table"));

        XMLAttribute* attr = table->GetAttributes();
        CHECK(attr);
        CHECK(Equal(attr->GetName(), "rows"));
        CHECK(Equal(attr->GetValue(), "3"));

        attr = attr->GetNext();
        CHECK(attr);
        CHECK(Equal(attr->GetName(), "border-radius"));
        CHECK(Equal(attr->GetValue(), "10px"));

        attr = attr->GetNext();
        CHECK(!attr);

        XMLElement* row = table->GetChild()->ToElement();
        CHECK(row);
        CHECK(Equal(row->GetName(), "tr"));

        XMLText* text = row->GetChild()->ToText();
        CHECK(text);
        CHECK(Equal(text->GetText(), "row 1"));

        row = row->GetNext()->ToElement();
        CHECK(row);

        text = row->GetChild()->ToText();
        CHECK(text);
        CHECK(Equal(text->GetText(), "row 2"));

        row = row->GetNext()->ToElement();
        CHECK(row);

        text = row->GetChild()->ToText();
        CHECK(text);
        CHECK(Equal(text->GetText(), "row 3"));

        doc = nullptr;
    }
}

TEST_CASE("XML Mixed Content")
{
    XMLParserConfig config{};
    config.SkipHeader = true;

    XMLParser parser(config);

    {
        const char* xml = "<p>xml mixed <b>content</b> test</p>";
        Ref<XMLDocument> doc = parser.ParseString(xml, strlen(xml));
        CHECK(doc);

        XMLElement* p = doc->GetChild()->ToElement();
        CHECK(p);
        CHECK(Equal(p->GetName(), "p"));

        XMLNode* content = p->GetChild();
        CHECK(content);
        XMLText* text = content->ToText();
        CHECK(text);
        CHECK(Equal(text->GetText(), "xml mixed "));

        content = content->GetNext();
        CHECK(content);
        XMLElement* b = content->ToElement();
        CHECK(b);
        CHECK(Equal(b->GetName(), "b"));

        text = b->GetChild()->ToText();
        CHECK(text);
        CHECK(Equal(text->GetText(), "content"));

        content = content->GetNext();
        CHECK(content);
        text = content->ToText();
        CHECK(text);
        CHECK(Equal(text->GetText(), " test"));

        doc = nullptr;
    }
}

TEST_CASE("Doxygen XML")
{
    const char* xml = R"(
  <compounddef id="class_l_d_1_1_view" kind="class" language="C++" prot="public">
    <compoundname>LD::View</compoundname>
    <templateparamlist>
      <param>
        <type>typename T</type>
      </param>
    </templateparamlist>
    <sectiondef kind="private-attrib">
      <memberdef kind="variable" id="class_l_d_1_1_view_1a6bfb0da9f675fa2cf64abb577b3b9147" prot="private" static="no" mutable="no">
        <type>const T *</type>
        <definition>const T* LD::View&lt; T &gt;::mData</definition>
        <argsstring></argsstring>
        <name>mData</name>
        <qualifiedname>LD::View::mData</qualifiedname>
        <briefdescription>
        </briefdescription>
        <detaileddescription>
        </detaileddescription>
        <inbodydescription>
        </inbodydescription>
        <location file="Ludens/Core/DSA/Include/View.h" line="38" column="9" bodyfile="Ludens/Core/DSA/Include/View.h" bodystart="38" bodyend="-1"/>
      </memberdef>
    </sectiondef>
  </compounddef>
)";

    XMLParserConfig config{};
    config.SkipHeader = true;

    XMLParser parser(config);
    {
        Ref<XMLDocument> doc = parser.ParseString(xml, strlen(xml));
        CHECK(doc);

        XMLElement* compounddef = doc->GetChild()->ToElement();
        CHECK(compounddef);
        CHECK(compounddef->GetChild());
        CHECK(!compounddef->GetNext());
        CHECK(Equal(compounddef->GetName(), "compounddef"));

        XMLElement* compoundname = compounddef->GetChild()->ToElement();
        CHECK(compoundname);
        CHECK(compoundname->GetNext());
        CHECK(Equal(compoundname->GetName(), "compoundname"));

        XMLElement* templateparamlist = compoundname->GetNext()->ToElement();
        CHECK(templateparamlist);
        CHECK(templateparamlist->GetNext());
        CHECK(templateparamlist->GetChild());
        CHECK(Equal(templateparamlist->GetName(), "templateparamlist"));

        XMLElement* sectiondef = templateparamlist->GetNext()->ToElement();
        CHECK(sectiondef);
        CHECK(sectiondef->GetChild());
        CHECK(Equal(sectiondef->GetName(), "sectiondef"));

        XMLElement* memberdef = sectiondef->GetChild()->ToElement();
        CHECK(memberdef);
        CHECK(memberdef->GetChild());
        CHECK(!memberdef->GetNext());
        CHECK(Equal(memberdef->GetName(), "memberdef"));

        XMLElement* type = memberdef->GetChild()->ToElement();
        CHECK(type);
        CHECK(type->GetNext());
        CHECK(type->GetChild());
        CHECK(Equal(type->GetName(), "type"));

        XMLElement* definition = type->GetNext()->ToElement();
        CHECK(definition);
        CHECK(definition->GetNext());
        CHECK(definition->GetChild());
        CHECK(Equal(definition->GetName(), "definition"));

        XMLElement* argsstring = definition->GetNext()->ToElement();
        CHECK(argsstring);
        CHECK(argsstring->GetNext());
        CHECK(!argsstring->GetChild());
        CHECK(Equal(argsstring->GetName(), "argsstring"));

        XMLElement* name = argsstring->GetNext()->ToElement();
        CHECK(name);
        CHECK(name->GetNext());
        CHECK(name->GetChild());
        CHECK(Equal(name->GetName(), "name"));

        doc = nullptr;
    }
}