#include <Ludens/DSA/Array.h>
#include <Ludens/DSA/HashMap.h>
#include <Ludens/DSA/Vector.h>
#include <Ludens/Header/KeyValue.h>
#include <Ludens/Header/MouseValue.h>
#include <Ludens/Memory/Memory.h>
#include <Ludens/Profiler/Profiler.h>
#include <Ludens/UI/UIImmediate.h>
#include <LudensEditor/DocumentWindow/DocumentWindow.h>
#include <LudensEditor/EditorContext/EditorIconAtlas.h>
#include <LudensEditor/EditorContext/EditorWindow.h>
#include <LudensEditor/EditorWidget/EUIDocument.h>

namespace LD {

/// @brief Editor document window implementation.
struct DocumentWindowObj : EditorWindowObj
{
    EUIDocumentStorage documentStorage;
    Document document = {};

    DocumentWindowObj(const EditorWindowInfo& info)
        : EditorWindowObj(info)
    {
        static const char source[] = R"(
# Lua API Core

```lua
function foo()
	return 3, 4
end
```

1. list item1
2. list item2
3. list item3

This is a [link](ld://Doc/Manual/GettingStarted/index.md) to another doc.

)";
        std::string err;
        DocumentInfo docI{};
        docI.md = View(source, sizeof(source) - 1);
        docI.uri = "doc://test.md";
        document = Document::create(docI, err);
        documentStorage.build(document);
    }

    void update(float delta);
};

void DocumentWindowObj::update(float delta)
{
    LD_PROFILE_SCOPE;

    begin_update_window();

    eui_document(&documentStorage);

    if (!documentStorage.requestURI.empty())
    {
        auto* event = (EditorRequestDocumentEvent*)ctx.enqueue_event(EDITOR_EVENT_TYPE_REQUEST_DOCUMENT);
        event->uri = documentStorage.requestURI;
        documentStorage.requestURI.clear();
    }

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

void DocumentWindow::update(EditorWindowObj* base, const EditorUpdateTick& tick)
{
    auto* obj = static_cast<DocumentWindowObj*>(base);

    obj->update(tick.delta);
}

} // namespace LD