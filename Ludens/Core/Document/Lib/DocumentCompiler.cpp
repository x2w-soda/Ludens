#include "Core/Document/Include/DocumentCompiler.h"
#include "Core/Serialize/Include/XML.h"

namespace LD
{

namespace Doxygen
{

// NOTE: the doxygen output parsing will mostly be invoked
//       by the Ludens Builder, so we can afford to spam
//       debug assertions to crash early in debug builds.

class CompoundDef
{
public:
    XMLString Name;
    XMLString ID;
    Vector<XMLElement*> PrivateAttribMembers;
    Vector<XMLElement*> PublicAttribMembers;
    Vector<XMLElement*> PublicFuncMembers;
    Vector<XMLElement*> PublicTypeMembers;
    Vector<XMLElement*> FriendMembers;

    void WriteMD(std::string& md);

private:
    void WriteMemberDefFn(XMLElement* memberdef, std::string& md);
    void WriteMemberDefType(XMLElement* type, std::string& md);
    void WriteMemberDefFnParam(XMLElement* param, std::string& md);
};

void CompoundDef::WriteMD(std::string& md)
{
    md.clear();

    md += "## " + std::string{ Name.Data(), Name.Size() } + "\n\n";

    if (!PublicFuncMembers.IsEmpty())
    {
        md += "### Public Functions\n\n";

        for (XMLElement* member : PublicFuncMembers)
            WriteMemberDefFn(member, md);
    }
}

void CompoundDef::WriteMemberDefFn(XMLElement* memberdef, std::string& md)
{
    LD_DEBUG_ASSERT(memberdef && memberdef->HasName("memberdef"));
    XMLNode* child = memberdef->GetChild();
    XMLString name;

    md += "#### ";

    // function return type
    XMLElement* type = nullptr;
    while (child && (type = child->ToElement()) && !type->HasName("type"))
        child = child->GetNext();

    WriteMemberDefType(type, md);
    md += ' ';

    // function name
    XMLElement* fnname = nullptr;
    while (child && (fnname = child->ToElement()) && !fnname->HasName("name"))
        child = child->GetNext();
    LD_DEBUG_ASSERT(fnname && fnname->HasName("name"));

    name = fnname->GetChild()->ToText()->GetText();
    md += std::string(name.Data(), name.Size()) + '(';

    // function qualifiedname
    child = child->GetNext();
    LD_DEBUG_ASSERT(child);

    // function params
    child = child->GetNext();
    LD_DEBUG_ASSERT(child);

    int numParams = 0;

    XMLElement* param;
    while (child && (param = child->ToElement()) && param->HasName("param"))
    {
        if (numParams++)
            md += ", ";

        WriteMemberDefFnParam(param, md);
        child = child->GetNext();
    }

    md += ")\n";
}

void CompoundDef::WriteMemberDefType(XMLElement* type, std::string& md)
{
    LD_DEBUG_ASSERT(type && type->HasName("type"));

    // Doxygen <type></type> has mixed content of text and <ref></ref> elements

    XMLNode* child;
    XMLText* text;
    XMLElement* ref;

    for (child = type->GetChild(); child; child = child->GetNext())
    {
        if ((text = child->ToText()))
        {
            XMLString str = text->GetText();
            md += std::string(str.Data(), str.Size());
        }
        else if ((ref = child->ToElement()) && ref->HasName("ref"))
        {
            XMLAttribute* refid = ref->GetAttributes();
            LD_DEBUG_ASSERT(refid->HasName("refid"));

            XMLNode* refchild = ref->GetChild();
            XMLString str = refchild->ToText()->GetText();
            XMLString href = refid->GetValue();

            md += '[' + std::string(str.Data(), str.Size()) + ']';
            md += '(' + std::string(href.Data(), href.Size()) + ')';
        }
        else
            LD_DEBUG_UNREACHABLE;
    }
}

void CompoundDef::WriteMemberDefFnParam(XMLElement* param, std::string& md)
{
    LD_DEBUG_ASSERT(param && param->HasName("param"));

    XMLNode* child = param->GetChild();
    LD_DEBUG_ASSERT(child);

    WriteMemberDefType(child->ToElement(), md);

    child = child->GetNext();
    LD_DEBUG_ASSERT(child);

    XMLElement* decl = child->ToElement();
    LD_DEBUG_ASSERT(decl && decl->HasName("declname"));

    XMLText* declname = decl->GetChild()->ToText();
    XMLString name = declname->GetText();
    md += ' ' + std::string(name.Data(), name.Size());
}

static bool OnCompoundName(XMLElement* compoundname, CompoundDef& def)
{
    if (!compoundname)
        return false;

    XMLNode* content = compoundname->GetChild();
    if (!content)
        return false;

    XMLText* text = content->ToText();
    if (!text)
        return false;

    def.Name = text->GetText();
    return true;
}

static bool OnTemplateParamList(XMLElement* templateparamlist, CompoundDef& def)
{
    if (!templateparamlist)
        return false;

    // TODO:
    return true;
}

static bool OnSectionDef(XMLElement* sectiondef, CompoundDef& def)
{
    if (!sectiondef)
        return false;

    XMLAttribute* attr = sectiondef->GetAttributes();
    if (!attr || !attr->HasName("kind"))
        return false;

    XMLString kind = attr->GetValue();
    Vector<XMLElement*>* section = nullptr;

    if (kind == "private-attrib")
        section = &def.PrivateAttribMembers;
    else if (kind == "public-attrib")
        section = &def.PublicAttribMembers;
    else if (kind == "public-func")
        section = &def.PublicFuncMembers;
    else if (kind == "public-type")
        section = &def.PublicTypeMembers;
    else if (kind == "friend")
        section = &def.FriendMembers;
    else
    {
        LD_DEBUG_UNREACHABLE;
        return false;
    }

    for (XMLNode* node = sectiondef->GetChild(); node; node = node->GetNext())
    {
        XMLElement* child;

        if (!(child = node->ToElement()))
            return false;

        if (!child->HasName("memberdef"))
        {
            LD_DEBUG_UNREACHABLE;
            return false;
        }

        section->PushBack(child);
    }

    return true;
}

static bool OnCompoundDef(XMLElement* compounddef, CompoundDef& def)
{
    if (!compounddef)
        return false;

    for (XMLAttribute* attr = compounddef->GetAttributes(); attr; attr = attr->GetNext())
    {
        if (attr->HasName("id"))
        {
            def.ID = attr->GetValue();
            break;
        }
    }

    bool valid = true;

    for (XMLNode* node = compounddef->GetChild(); node && valid; node = node->GetNext())
    {
        XMLElement* child;

        if (!(child = node->ToElement()))
            return false;

        if (child->HasName("compoundname"))
            valid = OnCompoundName(child, def);
        else if (child->HasName("templateparamlist"))
            valid = OnTemplateParamList(child, def);
        else if (child->HasName("sectiondef"))
            valid = OnSectionDef(child, def);
        else if (child->HasName("briefdescription") || child->HasName("detaileddescription") ||
                 child->HasName("location") || child->HasName("listofallmembers"))
            continue;
        else
        {
            LD_DEBUG_UNREACHABLE;
            return false;
        }
    }

    return valid;
}

} // namespace Doxygen

bool DocumentCompiler::CompileDoxygenXML(const char* xml, std::string& md)
{
    XMLParserConfig config;
    config.SkipHeader = false;

    XMLParser parser(config);
    Ref<XMLDocument> doc = parser.ParseString(xml, strlen(xml));

    XMLNode* node = doc->GetChild();
    XMLElement* doxygen = node->ToElement();
    LD_DEBUG_ASSERT(doxygen);

    node = doxygen->GetChild();
    XMLElement* compounddef = node->ToElement();

    Doxygen::CompoundDef def{};
    bool valid = Doxygen::OnCompoundDef(node->ToElement(), def);

    if (valid)
        def.WriteMD(md);

    return valid;
}

} // namespace LD
