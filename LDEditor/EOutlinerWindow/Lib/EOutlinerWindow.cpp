#include <Ludens/System/Memory.h>
#include <LudensEditor/EOutlinerWindow/EOutlinerWindow.h>
#include <LudensEditor/EditorContext/EditorWindowObj.h>
#include <iostream>

#define OUTLINER_ROW_SIZE 20.0f
#define OUTLINER_ROW_ODD_COLOR 0x272727FF
#define OUTLINER_ROW_EVEN_COLOR 0x2B2C2FFF

namespace LD {

struct OutlinerRow;

/// @brief Editor outliner window implementation.
struct EOutlinerWindowObj : EditorWindowObj
{
    virtual ~EOutlinerWindowObj() = default;

    std::vector<OutlinerRow*> rows;

    static void on_draw(UIWidget widget, ScreenRenderComponent renderer);
    static void on_window_resize(UIWindow window, const Vec2& size);
};

/// @brief A single row in the outliner window
struct OutlinerRow
{
    EditorContext editorCtx;
    UIPanelWidget panelWidget; /// row panel
    UIWidget textWidget;       /// data object name label
    DUID component;            /// the data component this row represents
    Color parityColor;
    int rowIndex;

    static OutlinerRow* create(EditorContext ctx, UINode parent, DUID component, int rowIndex)
    {
        EditorTheme theme = ctx.get_settings().get_theme();

        OutlinerRow* row = heap_new<OutlinerRow>(MEMORY_USAGE_UI);
        row->component = component;
        row->editorCtx = ctx;
        row->rowIndex = rowIndex;
        row->parityColor = (rowIndex % 2) ? OUTLINER_ROW_ODD_COLOR : OUTLINER_ROW_EVEN_COLOR;

        UILayoutInfo layoutI{};
        layoutI.childAxis = UI_AXIS_X;
        layoutI.sizeX = UISize::grow();
        layoutI.sizeY = UISize::fixed(OUTLINER_ROW_SIZE);

        UIPanelWidgetInfo panelI{};
        panelI.color = row->parityColor;
        row->panelWidget = parent.add_panel(layoutI, panelI, row);
        row->panelWidget.set_on_mouse_down(&OutlinerRow::on_mouse_down);

        UITextWidgetInfo textI{};
        textI.hoverHL = true;
        textI.cstr = component ? row->editorCtx.get_component_name(component) : "DataComponent";
        theme.get_font_size(textI.fontSize);
        row->textWidget = row->panelWidget.node().add_text(layoutI, textI, row);
        row->textWidget.set_on_mouse_down(&OutlinerRow::on_mouse_down);

        return row;
    }

    static void on_mouse_down(UIWidget widget, const Vec2& pos, MouseButton btn)
    {
        OutlinerRow& self = *(OutlinerRow*)widget.get_user();

        if (!self.component)
            return;

        self.editorCtx.set_selected_component(self.component);
    }

    void draw(EditorTheme editorTheme, ScreenRenderComponent renderer)
    {
        Color panelColor = parityColor;
        UITheme theme = editorTheme.get_ui_theme();

        if (component && component == editorCtx.get_selected_component())
            panelColor = 0x4D6490FF;

        panelWidget.set_panel_color(panelColor);
        panelWidget.on_draw(renderer);

        if (!component)
            return;

        textWidget.on_draw(renderer);
    }
};

void EOutlinerWindowObj::on_draw(UIWidget widget, ScreenRenderComponent renderer)
{
    auto& self = *(EOutlinerWindowObj*)widget.get_user();
    Rect windowRect = widget.get_rect();
    EditorTheme editorTheme = self.editorCtx.get_settings().get_theme();

    renderer.push_scissor(windowRect);

    for (OutlinerRow* row : self.rows)
    {
        row->draw(editorTheme, renderer);
    }

    renderer.pop_scissor();
}

void EOutlinerWindowObj::on_window_resize(UIWindow window, const Vec2& size)
{
    auto& self = *(EOutlinerWindowObj*)window.get_user();

    int rowCount = size.y / OUTLINER_ROW_SIZE + 1;

    for (int i = (int)self.rows.size(); i < rowCount; i++)
    {
        OutlinerRow* row = OutlinerRow::create(self.editorCtx, self.root.node(), 0, i);
        self.rows.push_back(row);
    }
}

EOutlinerWindow EOutlinerWindow::create(const EOutlinerWindowInfo& windowI)
{
    UIWindowManager wm = windowI.wm;
    EOutlinerWindowObj* obj = heap_new<EOutlinerWindowObj>(MEMORY_USAGE_UI);

    obj->root = wm.get_area_window(windowI.areaID);
    obj->root.set_user(obj);
    obj->root.set_on_draw(&EOutlinerWindowObj::on_draw);
    obj->editorCtx = windowI.ctx;

    wm.set_window_title(windowI.areaID, "Outliner");
    wm.set_on_window_resize(windowI.areaID, &EOutlinerWindowObj::on_window_resize);

    // create one OutlinerRow for each object in scene
    std::vector<DUID> sceneRoots;
    obj->editorCtx.get_scene_roots(sceneRoots);

    Rect rect = obj->root.get_rect();
    int rowCount = rect.h / OUTLINER_ROW_SIZE + 1;

    for (int i = 0; i < rowCount; i++)
    {
        DUID root = i < sceneRoots.size() ? sceneRoots[i] : 0;
        OutlinerRow* row = OutlinerRow::create(obj->editorCtx, obj->root.node(), root, i);
        obj->rows.push_back(row);
    }

    return {obj};
}

void EOutlinerWindow::destroy(EOutlinerWindow window)
{
    EOutlinerWindowObj* obj = window;

    heap_delete<EOutlinerWindowObj>(obj);
}

} // namespace LD