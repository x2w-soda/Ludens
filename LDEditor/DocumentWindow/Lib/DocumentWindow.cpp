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
    }

    virtual EditorWindowType get_type() override { return EDITOR_WINDOW_DOCUMENT; }
    virtual void on_imgui(float delta) override;
};

void DocumentWindowObj::on_imgui(float delta)
{
    LD_PROFILE_SCOPE;

    ui_workspace_begin();
    ui_push_window(ui_workspace_name());

    eui_document(&documentStorage);

    ui_pop_window();
    ui_workspace_end();
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
    LD_ASSERT(window && window.get_type() == EDITOR_WINDOW_DOCUMENT);

    auto* obj = static_cast<DocumentWindowObj*>(window.unwrap());

    heap_delete<DocumentWindowObj>(obj);
}

} // namespace LD