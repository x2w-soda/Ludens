#include <Ludens/System/Memory.h>
#include <LudensEditor/EOutlinerWindow/EOutlinerWindow.h>
#include <LudensEditor/EditorContext/EditorWindowObj.h>
#include <iostream>

#define OUTLINER_ROW_SIZE 20.0f
#define OUTLINER_ROW_ODD_COLOR 0x272727FF
#define OUTLINER_ROW_EVEN_COLOR 0x2B2C2FFF
#define OUTLINER_ROW_LEFT_PADDING 10.0f
#define OUTLINER_ROW_LEFT_PADDING_PER_DEPTH 15.0f

namespace LD {

struct OutlinerRow;

/// @brief Editor outliner window implementation.
struct EOutlinerWindowObj : EditorWindowObj
{
    virtual ~EOutlinerWindowObj() = default;

    std::vector<OutlinerRow*> rowOrder; /// rows ordered top to bottom

    OutlinerRow* get_or_create_row(int rowIdx, int depth, DUID compID);
    void invalidate();
    void invalidate_component(const ComponentBase* base, int& rowIdx, int depth);

    static void on_window_resize(UIWindow window, const Vec2& size);
};

/// @brief A single row in the outliner window
struct OutlinerRow
{
    EditorContext editorCtx;
    UIPanelWidget panelWidget; /// row panel
    UITextWidget textWidget;   /// data object name label
    DUID component;            /// the data component this row represents
    Color parityColor;
    int rowIndex;

    void display(DUID compID, int depth)
    {
        component = compID;
        const ComponentBase* base = editorCtx.get_component_base(compID);
        textWidget.set_text(base ? base->name : nullptr);

        UIPadding padding{};
        padding.left = OUTLINER_ROW_LEFT_PADDING;
        padding.left += depth * OUTLINER_ROW_LEFT_PADDING_PER_DEPTH;
        panelWidget.set_layout_child_padding(padding);
    }

    static OutlinerRow* create(EditorContext ctx, UINode parentNode, DUID component, int rowIndex)
    {
        EditorTheme theme = ctx.get_settings().get_theme();

        OutlinerRow* row = heap_new<OutlinerRow>(MEMORY_USAGE_UI);
        row->component = component;
        row->editorCtx = ctx;
        row->rowIndex = rowIndex;
        row->parityColor = (rowIndex % 2) ? OUTLINER_ROW_ODD_COLOR : OUTLINER_ROW_EVEN_COLOR;

        UILayoutInfo layoutI{};
        layoutI.childAxis = UI_AXIS_X;
        layoutI.childPadding.left = OUTLINER_ROW_LEFT_PADDING;
        layoutI.sizeX = UISize::grow();
        layoutI.sizeY = UISize::fixed(OUTLINER_ROW_SIZE);

        UIPanelWidgetInfo panelI{};
        panelI.color = row->parityColor;
        row->panelWidget = parentNode.add_panel(layoutI, panelI, row);
        row->panelWidget.set_on_mouse_down(&OutlinerRow::on_mouse_down);
        row->panelWidget.set_on_draw(&OutlinerRow::on_draw);

        layoutI.childPadding = {};
        UITextWidgetInfo textI{};
        textI.hoverHL = true;
        textI.cstr = component ? row->editorCtx.get_component_name(component) : nullptr;
        theme.get_font_size(textI.fontSize);
        row->textWidget = row->panelWidget.node().add_text(layoutI, textI, row);
        row->textWidget.set_on_mouse_down(&OutlinerRow::on_mouse_down);

        return row;
    }

    static void on_draw(UIWidget widget, ScreenRenderComponent renderer)
    {
        OutlinerRow& self = *(OutlinerRow*)widget.get_user();
        Color color = self.parityColor;
        Rect rect = self.panelWidget.get_rect();

        if (self.component && self.component == self.editorCtx.get_selected_component())
            color = 0x4D6490FF;

        renderer.draw_rect(rect, color);
    }

    static void on_mouse_down(UIWidget widget, const Vec2& pos, MouseButton btn)
    {
        OutlinerRow& self = *(OutlinerRow*)widget.get_user();

        if (!self.component)
            return;

        self.editorCtx.set_selected_component(self.component);
    }
};

OutlinerRow* EOutlinerWindowObj::get_or_create_row(int rowIdx, int depth, DUID compID)
{
    if (0 <= rowIdx && rowIdx < (int)rowOrder.size())
    {
        OutlinerRow* row = rowOrder[rowIdx];
        row->display(compID, depth);
        return row;
    }

    rowOrder.resize(rowIdx + 1); // TODO: this may leave unitialized gaps?
    OutlinerRow* row = OutlinerRow::create(editorCtx, root.node(), compID, rowIdx);
    row->display(compID, depth);

    return rowOrder[rowIdx] = row;
}

void EOutlinerWindowObj::invalidate()
{
    std::vector<DUID> sceneRoots;
    editorCtx.get_scene_roots(sceneRoots);

    Rect rect = root.get_rect();

    int depth = 0;
    int rowIdx = 0;

    for (DUID sceneRoot : sceneRoots)
    {
        const ComponentBase* base = editorCtx.get_component_base(sceneRoot);
        invalidate_component(base, rowIdx, depth);
    }

    int rowCount = rect.h / OUTLINER_ROW_SIZE + 1;
    for (; rowIdx < rowCount; rowIdx++)
    {
        get_or_create_row(rowIdx, 0, (DUID)0);
    }
}

void EOutlinerWindowObj::invalidate_component(const ComponentBase* base, int& rowIdx, int depth)
{
    LD_ASSERT(base);

    get_or_create_row(rowIdx++, depth, base->id);

    depth++;
    for (const ComponentBase* child = base->child; child; child = child->next)
    {
        invalidate_component(child, rowIdx, depth);
    }
    depth--;
}

void EOutlinerWindowObj::on_window_resize(UIWindow window, const Vec2& size)
{
    auto& self = *(EOutlinerWindowObj*)window.get_user();

    int rowCount = size.y / OUTLINER_ROW_SIZE + 1;

    for (int i = (int)self.rowOrder.size(); i < rowCount; i++)
    {
        self.get_or_create_row(i, 0, (DUID)0);
    }
}

EOutlinerWindow EOutlinerWindow::create(const EOutlinerWindowInfo& windowI)
{
    UIWindowManager wm = windowI.wm;
    EOutlinerWindowObj* obj = heap_new<EOutlinerWindowObj>(MEMORY_USAGE_UI);

    obj->root = wm.get_area_window(windowI.areaID);
    obj->root.set_user(obj);
    obj->editorCtx = windowI.ctx;

    wm.set_window_title(windowI.areaID, "Outliner");
    wm.set_on_window_resize(windowI.areaID, &EOutlinerWindowObj::on_window_resize);

    // create one OutlinerRow for each object in scene
    obj->invalidate();

    return {obj};
}

void EOutlinerWindow::destroy(EOutlinerWindow window)
{
    EOutlinerWindowObj* obj = window;

    heap_delete<EOutlinerWindowObj>(obj);
}

} // namespace LD