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
#include <LudensEditor/EditorWidget/EUIText.h>

namespace LD {

/// @brief Editor document window implementation.
struct DocumentWindowObj : EditorWindowObj
{
    EUIDocument document;
    EUITextBreadcrumb breadcrumb;
    std::string currentURIPath;

    DocumentWindowObj(const EditorWindowInfo& info)
        : EditorWindowObj(info)
    {
    }

    void build(Document doc);
    void pre_update();
    void update();
    void top_bar();
};

void DocumentWindowObj::build(Document doc)
{
    if (!doc)
        return;

    currentURIPath = doc.get_uri_path();
    document.build(doc);

    std::string str = doc.get_uri_path();
    breadcrumb.build(str.c_str());
}

void DocumentWindowObj::pre_update()
{
    if (currentURIPath.empty())
        document.set_request_uri_path(document_uri_default_page_path());

    std::string requestURIPath;
    if (document.get_request_uri_path(requestURIPath) && requestURIPath != currentURIPath)
    {
        Document doc = ctx.get_document(requestURIPath.c_str());

        // Rebuild document before imgui pass.
        build(doc);
    }
}

void DocumentWindowObj::update()
{
    LD_PROFILE_SCOPE;

    begin_update_window();
    ui_top_layout_child_axis(UI_AXIS_Y);

    top_bar();

    document.push();
    document.pop();

    end_update_window();
}

void DocumentWindowObj::top_bar()
{
    float barHeight = theme.get_text_row_height();
    Color bgColor = theme.get_ui_theme().get_surface_color();

    UIPanelData* panel = (UIPanelData*)ui_push_panel(nullptr, bgColor).get_data();
    UILayoutInfo layoutI = theme.make_hbox_layout();
    layoutI.sizeX = UISize::grow();
    ui_top_layout(layoutI);

    std::string path = breadcrumb.update(barHeight, 0x20FFFFFF);
    if (!path.empty())
        document.set_request_uri_path(path);

    ui_pop();
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