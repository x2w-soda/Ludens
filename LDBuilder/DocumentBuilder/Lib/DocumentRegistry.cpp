#include <Ludens/Memory/Memory.h>
#include <LudensBuilder/DocumentBuilder/DocumentRegistry.h>

#include <format>

namespace LD {

struct DocumentRegistryObj
{
    HashMap<std::string, Document> docs;
};

DocumentRegistry DocumentRegistry::create()
{
    auto* obj = heap_new<DocumentRegistryObj>(MEMORY_USAGE_MISC);

    return DocumentRegistry(obj);
}

void DocumentRegistry::destroy(DocumentRegistry registry)
{
    auto* obj = registry.unwrap();

    for (auto it : obj->docs)
        Document::destroy(it.second);

    heap_delete<DocumentRegistryObj>(obj);
}

bool DocumentRegistry::add_document(const DocumentInfo& info, std::string& err)
{
    if (mObj->docs.contains(info.uri))
    {
        err = std::format("document with uri [{}] already exists", info.uri);
        return false;
    }

    Document doc = Document::create(info, err);
    if (!doc)
        return false;

    mObj->docs[info.uri] = doc;

    return true;
}

Document DocumentRegistry::get_document(const char* uri)
{
    auto it = mObj->docs.find(uri);

    if (it == mObj->docs.end())
        return {};

    return it->second;
}

} // namespace LD