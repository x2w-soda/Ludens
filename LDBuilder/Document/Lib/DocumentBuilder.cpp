#include <Ludens/DSA/HashMap.h>
#include <Ludens/Header/Hash.h>
#include <Ludens/Media/Format/XML.h>
#include <Ludens/Memory/Allocator.h>
#include <LudensBuilder/Document/DocumentBuilder.h>

#include <string>

namespace LD {

enum CppItemType
{
    CPP_ITEM_INVALID = 0,
    CPP_ITEM_CLASS,
    CPP_ITEM_STRUCT,
    CPP_ITEM_VARIABLE,
    CPP_ITEM_FUNCTION,
};

struct CppCompound
{
    View name;
    View refid;
    CppItemType type;
};

struct CppMember
{
    CppMember* next;
    View name;
    View refid;
    CppItemType type;
};

static CppItemType get_item_type(const View& view)
{
    if (view == "class")
        return CPP_ITEM_CLASS;

    if (view == "struct")
        return CPP_ITEM_STRUCT;

    if (view == "variable")
        return CPP_ITEM_VARIABLE;

    if (view == "function")
        return CPP_ITEM_FUNCTION;

    return CPP_ITEM_INVALID;
}

/// @brief Document builder implementation. For C++ documentation we are
///        using the XML output of Doxygen.
struct DocumentBuilderObj
{
    XMLDocument indexXML;
    PoolAllocator cppCompoundPA;
    PoolAllocator cppMmeberPA;
    HashMap<std::string, CppCompound*> cppClasses;
    HashMap<std::string, CppCompound*> cppStructs;
    HashMap<std::string, CppCompound*> cppVariables;
    HashMap<std::string, CppCompound*> cppFunctions;
};

DocumentBuilder DocumentBuilder::create(const DocumentBuilderInfo& compilerI)
{
    DocumentBuilderObj* obj = heap_new<DocumentBuilderObj>(MEMORY_USAGE_MISC);

    // using create_from_file ensures that all string views point to the file buffer,
    // which is valid until the xml document itself is destroyed.
    obj->indexXML = XMLDocument::create_from_file(compilerI.doxygenXMLPath);

    PoolAllocatorInfo paI{};
    paI.blockSize = sizeof(CppCompound);
    paI.pageSize = 128;
    paI.isMultiPage = true;
    paI.usage = MEMORY_USAGE_MISC;
    obj->cppCompoundPA = PoolAllocator::create(paI);

    paI.blockSize = sizeof(CppMember);
    obj->cppMmeberPA = PoolAllocator::create(paI);

    XMLElement doxygenindex = obj->indexXML.get_root();

    XMLString str;
    for (XMLElement compound = doxygenindex.get_child(str); compound; compound = compound.get_next(str))
    {
        XMLString compoundRefid{};
        CppItemType compoundType = CPP_ITEM_INVALID;

        for (XMLAttribute attr = compound.get_attributes(); attr; attr = attr.get_next())
        {
            XMLString attrName = attr.get_name();
            if (attrName == "refid")
                compoundRefid = attr.get_value();
            else if (attrName == "kind")
                compoundType = get_item_type(attr.get_value());
        }

        if (compoundRefid.size == 0 || compoundType == CPP_ITEM_INVALID)
            continue; // TODO: warn ignored compound

        std::string refidKey(compoundRefid.data, (int)compoundRefid.size);
        CppCompound* comp = (CppCompound*)obj->cppCompoundPA.allocate();
        comp->name = {};
        comp->refid = compoundRefid;

        XMLElement compoundName = compound.get_child(str);
        if (compoundName && compoundName.get_name() == "name")
        {
            compoundName.get_child(comp->name);
            // printf("compound: %.*s\n", (unsigned int)str.size(), str.data());
        }

        switch (compoundType)
        {
        case CPP_ITEM_CLASS:
            obj->cppClasses[refidKey] = comp;
            break;
        case CPP_ITEM_STRUCT:
            obj->cppStructs[refidKey] = comp;
            break;
        case CPP_ITEM_VARIABLE:
            obj->cppVariables[refidKey] = comp;
            break;
        case CPP_ITEM_FUNCTION:
            obj->cppFunctions[refidKey] = comp;
            break;
        default:
            break;
        }
    }

    printf("cpp classes: %d\n", (int)obj->cppClasses.size());
    printf("cpp structs: %d\n", (int)obj->cppStructs.size());
    printf("cpp variables: %d\n", (int)obj->cppVariables.size());
    printf("cpp functions: %d\n", (int)obj->cppFunctions.size());

    return {obj};
}

void DocumentBuilder::destroy(DocumentBuilder compiler)
{
    DocumentBuilderObj* obj = compiler;

    XMLDocument::destroy(obj->indexXML);

    heap_delete<DocumentBuilderObj>(obj);
}

} // namespace LD