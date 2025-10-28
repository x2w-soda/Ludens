#include "ComponentMenu.h"
#include <Ludens/Profiler/Profiler.h>
#include <Ludens/System/Memory.h>
#include <LudensEditor/EOutlinerWindow/EOutlinerWindow.h>
#include <LudensEditor/EditorContext/EditorIconAtlas.h>
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
    ComponentMenu menu;

    OutlinerRow* get_or_create_row(int rowIdx, int depth, CUID compID);
    void invalidate();
    void invalidate_component(const ComponentBase* base, int& rowIdx, int depth);

    virtual void on_draw_overlay(ScreenRenderComponent renderer) override;

    static void on_client_resize(UIWindow client, const Vec2& size, void* user);
};

/// @brief A single row in the outliner window
struct OutlinerRow
{
    EOutlinerWindowObj* outlinerWindow;
    EditorContext editorCtx;
    UIPanelWidget panelWidget; /// row panel
    UITextWidget textWidget;   /// data object name label
    UIImageWidget scriptIcon;  /// icon for component script
    CUID component;            /// the data component this row represents
    Color parityColor;
    int rowIndex;

    void display(CUID compID, int depth)
    {
        component = compID;
        const ComponentBase* base = editorCtx.get_component_base(compID);
        textWidget.set_text(base ? base->name : nullptr);

        const ComponentScriptSlot* script = editorCtx.get_component_script_slot(compID);
        if (script)
            scriptIcon.show();
        else
            scriptIcon.hide();

        UIPadding padding{};
        padding.left = OUTLINER_ROW_LEFT_PADDING;
        padding.left += depth * OUTLINER_ROW_LEFT_PADDING_PER_DEPTH;
        panelWidget.set_layout_child_padding(padding);
    }

    static OutlinerRow* create(EOutlinerWindowObj* obj, EditorContext ctx, UINode parentNode, CUID component, int rowIndex)
    {
        EditorTheme theme = ctx.get_settings().get_theme();

        OutlinerRow* row = heap_new<OutlinerRow>(MEMORY_USAGE_UI);
        row->outlinerWindow = obj;
        row->component = component;
        row->editorCtx = ctx;
        row->rowIndex = rowIndex;
        row->parityColor = (rowIndex % 2) ? OUTLINER_ROW_ODD_COLOR : OUTLINER_ROW_EVEN_COLOR;

        UILayoutInfo layoutI{};
        layoutI.childAxis = UI_AXIS_X;
        layoutI.childGap = theme.get_padding();
        layoutI.childPadding.left = OUTLINER_ROW_LEFT_PADDING;
        layoutI.sizeX = UISize::grow();
        layoutI.sizeY = UISize::fixed(OUTLINER_ROW_SIZE);

        UIPanelWidgetInfo panelI{};
        panelI.color = row->parityColor;
        row->panelWidget = parentNode.add_panel(layoutI, panelI, row);
        row->panelWidget.set_on_mouse(&OutlinerRow::on_mouse);
        row->panelWidget.set_on_draw(&OutlinerRow::on_draw);

        layoutI.childPadding = {};
        UITextWidgetInfo textI{};
        textI.hoverHL = true;
        textI.cstr = component ? row->editorCtx.get_component_name(component) : nullptr;
        textI.fontSize = theme.get_font_size();
        row->textWidget = row->panelWidget.node().add_text(layoutI, textI, row);
        row->textWidget.set_on_mouse(&OutlinerRow::on_mouse);

        layoutI.sizeX = UISize::fixed(OUTLINER_ROW_SIZE);
        layoutI.sizeY = UISize::fixed(OUTLINER_ROW_SIZE);
        Rect iconRect = EditorIconAtlas::get_icon_rect(EditorIcon::Code);
        UIImageWidgetInfo imageWI{};
        imageWI.image = row->editorCtx.get_editor_icon_atlas();
        imageWI.rect = &iconRect;
        row->scriptIcon = row->panelWidget.node().add_image(layoutI, imageWI, row);
        row->scriptIcon.hide();
        // TODO:

        return row;
    }

    static void on_draw(UIWidget widget, ScreenRenderComponent renderer)
    {
        OutlinerRow& self = *(OutlinerRow*)widget.get_user();
        Color color = self.parityColor;
        Rect rect = self.panelWidget.get_rect();
        EditorContext eCtx = self.outlinerWindow->editorCtx;
        UITheme theme = eCtx.get_theme().get_ui_theme();

        if (self.component && self.component == eCtx.get_selected_component())
            color = theme.get_selection_color();

        renderer.draw_rect(rect, color);
    }

    static void on_mouse(UIWidget widget, const Vec2& pos, MouseButton btn, UIEvent event)
    {
        OutlinerRow& self = *(OutlinerRow*)widget.get_user();

        if (!self.component)
            return;

        if (event == UI_MOUSE_DOWN)
        {
            if (btn == MOUSE_BUTTON_LEFT)
                self.editorCtx.set_selected_component(self.component);
            else if (btn == MOUSE_BUTTON_RIGHT)
            {
                Vec2 screenPos = widget.get_pos() + pos;
                self.outlinerWindow->menu.show(screenPos, self.component);
            }
        }
    }
};

OutlinerRow* EOutlinerWindowObj::get_or_create_row(int rowIdx, int depth, CUID compID)
{
    if (0 <= rowIdx && rowIdx < (int)rowOrder.size())
    {
        OutlinerRow* row = rowOrder[rowIdx];
        row->display(compID, depth);
        return row;
    }

    rowOrder.resize(rowIdx + 1); // TODO: this may leave unitialized gaps?
    OutlinerRow* row = OutlinerRow::create(this, editorCtx, root.node(), compID, rowIdx);
    row->display(compID, depth);

    return rowOrder[rowIdx] = row;
}

void EOutlinerWindowObj::invalidate()
{
    LD_PROFILE_SCOPE;

    std::vector<CUID> sceneRoots;
    editorCtx.get_scene_roots(sceneRoots);

    Rect rect = root.get_rect();

    int depth = 0;
    int rowIdx = 0;

    for (CUID sceneRoot : sceneRoots)
    {
        const ComponentBase* base = editorCtx.get_component_base(sceneRoot);
        invalidate_component(base, rowIdx, depth);
    }

    int rowCount = rect.h / OUTLINER_ROW_SIZE + 1;
    for (; rowIdx < rowCount; rowIdx++)
    {
        get_or_create_row(rowIdx, 0, (CUID)0);
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

void EOutlinerWindowObj::on_draw_overlay(ScreenRenderComponent renderer)
{
    menu.draw(renderer);
}

void EOutlinerWindowObj::on_client_resize(UIWindow client, const Vec2& size, void* user)
{
    auto& self = *(EOutlinerWindowObj*)client.get_user();

    int rowCount = size.y / OUTLINER_ROW_SIZE + 1;

    for (int i = (int)self.rowOrder.size(); i < rowCount; i++)
    {
        self.get_or_create_row(i, 0, (CUID)0);
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
    wm.set_resize_callback(windowI.areaID, &EOutlinerWindowObj::on_client_resize);
    UIWindow outlinerWindow = wm.get_area_window(windowI.areaID);
    outlinerWindow.set_user(obj);
    outlinerWindow.set_on_update([](UIWidget widget, float delta) {
        auto* obj = (EOutlinerWindowObj*)widget.get_user();
        obj->invalidate();
    });

    ComponentMenuInfo menuI{};
    menuI.ctx = wm.get_context();
    menuI.theme = obj->editorCtx.get_theme();
    menuI.onOptionAddScript = windowI.addScriptToComponent;
    menuI.user = windowI.user;
    menuI.layer = wm.get_ground_layer_hash();
    obj->menu.startup(menuI);

    // create one OutlinerRow for each object in scene
    obj->invalidate();

    return {obj};
}

void EOutlinerWindow::destroy(EOutlinerWindow window)
{
    EOutlinerWindowObj* obj = window;

    obj->menu.cleanup();

    heap_delete<EOutlinerWindowObj>(obj);
}

} // namespace LD