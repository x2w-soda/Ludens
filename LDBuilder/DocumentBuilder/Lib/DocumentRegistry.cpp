#include <Ludens/Header/Assert.h>
#include <Ludens/Memory/Memory.h>
#include <Ludens/Profiler/Profiler.h>
#include <LudensBuilder/DocumentBuilder/DocumentRegistry.h>
#include <LudensBuilder/DocumentBuilder/DocumentURI.h>

#include <format>

namespace LD {

struct DocumentRegistryObj
{
    HashMap<String, Document> docs;

    bool validate_doc_uris(const DocumentRefs& refs, String& docURI);
    bool validate_misc_uris(const DocumentRefs& refs, DocumentRegistryValidator& validator);
};

bool DocumentRegistryObj::validate_doc_uris(const DocumentRefs& refs, String& docURI)
{
    for (const View& api : refs.luaAPI)
    {
        docURI = String((const char*)api.data, api.size);

        URI uri(docURI);
        String path = document_uri_normalized_path(uri);

        if (!docs.contains(path))
            return false;
    }

    for (const View& manual : refs.manual)
    {
        docURI = String((const char*)manual.data, manual.size);

        URI uri(docURI);
        String path = document_uri_normalized_path(uri);

        if (!docs.contains(path))
            return false;
    }

    return true;
}

bool DocumentRegistryObj::validate_misc_uris(const DocumentRefs& refs, DocumentRegistryValidator& validator)
{
    for (const View& misc : refs.misc)
    {
        if (!validator.onMiscURI(misc))
        {
            validator.missingURI = String((const char*)misc.data, misc.size);
            return false;
        }
    }

    return true;
}

DocumentRegistry DocumentRegistry::create()
{
    auto* obj = heap_new<DocumentRegistryObj>(MEMORY_USAGE_DOCUMENT);

    return DocumentRegistry(obj);
}

void DocumentRegistry::destroy(DocumentRegistry registry)
{
    auto* obj = registry.unwrap();

    for (auto it : obj->docs)
        Document::destroy(it.second);

    heap_delete<DocumentRegistryObj>(obj);
}

bool DocumentRegistry::add_document(const DocumentInfo& info, String& err)
{
    LD_ASSERT(info.uriPath);
    String uriPath = info.uriPath;

    if (uriPath.empty())
    {
        err = "invalid uri";
        return false;
    }


    if (mObj->docs.contains(uriPath))
    {
        err = std::format("document with uri path [{}] already exists", uriPath).c_str();
        return false;
    }

    Document doc = Document::create(info, err);
    if (!doc)
        return false;

    mObj->docs[uriPath] = doc;

    return true;
}

Document DocumentRegistry::get_document(const char* uriPath)
{
    auto it = mObj->docs.find(uriPath);

    if (it == mObj->docs.end())
        return {};

    return it->second;
}

bool DocumentRegistry::validate(DocumentRegistryValidator& validator)
{
    LD_PROFILE_SCOPE;

    LD_ASSERT(validator.onMiscURI);

    for (const auto& it : mObj->docs)
    {
        Document doc = it.second;
        DocumentRefs refs = doc.get_references();

        if (!mObj->validate_doc_uris(refs, validator.missingURI))
            return false;

        if (!mObj->validate_misc_uris(refs, validator))
            return false;
    }

    return true;
}

} // namespace LD