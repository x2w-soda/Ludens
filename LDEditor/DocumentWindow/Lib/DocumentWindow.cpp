#include <Ludens/DSA/Array.h>
#include <Ludens/DSA/HashMap.h>
#include <Ludens/DSA/Vector.h>
#include <Ludens/Header/KeyValue.h>
#include <Ludens/Header/MouseValue.h>
#include <Ludens/Memory/Memory.h>
#include <Ludens/Profiler/Profiler.h>
#include <Ludens/UI/UIImmediate.h>
#include <LudensBuilder/DocumentBuilder/DocumentURI.h>
#include <LudensEditor/DocumentWindow/DocumentWindow.h>
#include <LudensEditor/EditorContext/EditorIconAtlas.h>
#include <LudensEditor/EditorContext/EditorWindow.h>
#include <LudensEditor/EditorWidget/EUIDocument.h>

namespace LD {

/// @brief Editor document window implementation.
struct DocumentWindowObj : EditorWindowObj
{
    EUIDocumentStorage documentStorage;
    std::string currentURI;

    DocumentWindowObj(const EditorWindowInfo& info)
        : EditorWindowObj(info)
    {
        /*
        std::string err;
        DocumentInfo docI{};
        docI.md = View(source, sizeof(source) - 1);
        docI.uri = "ld://Doc/Manual";
        Document doc = Document::create(docI, err);
        documentStorage.build(doc);
        */
    }

    void pre_update();
    void update();
};

void DocumentWindowObj::pre_update()
{
    if (currentURI.empty())
        documentStorage.requestURI = document_uri_default_page();

    if (!documentStorage.requestURI.empty() && documentStorage.requestURI != currentURI)
    {
        Document doc = ctx.get_document(documentStorage.requestURI.c_str());

        if (doc)
        {
            // Rebuild document before imgui pass.
            currentURI = documentStorage.requestURI;
            documentStorage.requestURI.clear();
            documentStorage.build(doc);
        }
    }
}

void DocumentWindowObj::update()
{
    LD_PROFILE_SCOPE;

    begin_update_window();

    eui_document(&documentStorage);

    end_update_window();
}

//
// Public API
//

EditorWindow DocumentWindow::create(const EditorWindowInfo& windowI)
{
    DocumentWindowObj* obj = heap_new<DocumentWindowObj>(MEMORY_USAGE_UI, windowI);

    return EditorWindow(obj);
}

void DocumentWindow::destroy(EditorWindow window)
{
    auto* obj = static_cast<DocumentWindowObj*>(window.unwrap());

    heap_delete<DocumentWindowObj>(obj);
}

void DocumentWindow::pre_update(EditorWindowObj* base, const EditorUpdateTick& tick)
{
    auto* obj = static_cast<DocumentWindowObj*>(base);

    (void)tick;
    obj->pre_update();
}

void DocumentWindow::update(EditorWindowObj* base, const EditorUpdateTick& tick)
{
    auto* obj = static_cast<DocumentWindowObj*>(base);

    (void)tick;
    obj->update();
}

} // namespace LD