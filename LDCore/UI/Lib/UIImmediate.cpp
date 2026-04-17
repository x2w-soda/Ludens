#include <Ludens/DSA/HashMap.h>
#include <Ludens/DSA/Stack.h>
#include <Ludens/DSA/Vector.h>
#include <Ludens/Header/Assert.h>
#include <Ludens/Header/Hash.h>
#include <Ludens/Header/Impulse.h>
#include <Ludens/Header/KeyValue.h>
#include <Ludens/Header/MouseValue.h>
#include <Ludens/Header/Types.h>
#include <Ludens/Memory/Allocator.h>
#include <Ludens/Profiler/Profiler.h>
#include <Ludens/UI/UIImmediate.h>
#include <Ludens/UI/Widgets.h>

#define LD_ASSERT_NOT_UI_CONTEXT_SCOPE LD_ASSERT(!sImContext && "ui_context_end not called")
#define LD_ASSERT_UI_CONTEXT_SCOPE LD_ASSERT(sImContext && "ui_context_begin not called")
#define LD_ASSERT_UI_LAYER_SCOPE LD_ASSERT(sImContext->imLayer && "ui_layer_begin not called")
#define LD_ASSERT_UI_WORKSPACE_SCOPE LD_ASSERT(sImContext->imSpace && "ui_workspace_begin not called")
#define LD_ASSERT_UI_WINDOW_SCOPE LD_ASSERT(sImContext->imWindow && "ui_push_window not called")

#define LD_ASSERT_UI_PUSH LD_ASSERT_UI_WINDOW_SCOPE

#define LD_ASSERT_UI_TOP_WIDGET \
    LD_ASSERT_UI_WINDOW_SCOPE

#define LD_ASSERT_UI_TOP_WIDGET_TYPE(TYPE) \
    LD_ASSERT_UI_TOP_WIDGET;               \
    LD_ASSERT(sImContext->imWindow->imWidgetStack.top()->type == (TYPE))

namespace LD {

struct UIWidgetState;
struct UIWindowState;
struct UIWorkspaceState;
struct UILayerState;
struct UIContextState;

struct UITextState
{
    Vector<int> spanClicks;

    static bool on_span_event(UIWidget widget, const UIEvent& event, UITextSpan& span, int spanIndex, void* user);
};

struct UITextEditState
{
    std::string lastChange;
    std::string lastSubmission;
    Impulse isChanged;
    Impulse isSubmitted;
};

struct UIDropdownState
{
    Impulse isOpened = {};
};

struct UIWidgetState
{
    UIWidget widget{};               // actual retained widget
    Vector<UIWidgetState*> children; // direct children, retained across frames
    UIWindowState* window = nullptr; // owning window state
    IMDrawCallback onDraw = nullptr; // imgui user draw callback
    Hash64 widgetHash{};             // hash that identifies this state uniquely in its window
    Vector<UIEvent> events;          // widget events in this frame
    void* imUser = nullptr;          // imgui user
    int childCounter = 0;            // number of children widget states in this frame
    const UIWidgetType type;
    union
    {
        Impulse isTogglePressed;
        Impulse isButtonPressed;
        UITextState text;
        UITextEditState textEdit;
        UIDropdownState dropdown;
    };

    UIWidgetState() = delete;
    UIWidgetState(UIWidgetType type);
    UIWidgetState(const UIWidgetState&) = delete;
    ~UIWidgetState();

    UIWidgetState& operator=(const UIWidgetState&) = delete;

    bool consume_event(UIEventType type, UIEvent& outEvent);
};

UIWidgetState::UIWidgetState(UIWidgetType type)
    : type(type)
{
    switch (type)
    {
    case UI_WIDGET_BUTTON:
        isButtonPressed.set(false);
        break;
    case UI_WIDGET_TOGGLE:
        isTogglePressed.set(false);
        break;
    case UI_WIDGET_TEXT:
        new (&text) UITextState();
        break;
    case UI_WIDGET_TEXT_EDIT:
        new (&textEdit) UITextEditState();
        break;
    case UI_WIDGET_DROPDOWN:
        new (&dropdown) UIDropdownState();
        break;
    default:
        break;
    }
}

UIWidgetState::~UIWidgetState()
{
    switch (type)
    {
    case UI_WIDGET_TEXT:
        (&text)->~UITextState();
        break;
    case UI_WIDGET_TEXT_EDIT:
        (&textEdit)->~UITextEditState();
        break;
    case UI_WIDGET_DROPDOWN:
        (&dropdown)->~UIDropdownState();
        break;
    default:
        break;
    }
}

bool UIWidgetState::consume_event(UIEventType type, UIEvent& outEvent)
{
    int consumeIdx = -1;

    for (size_t i = 0; i < events.size(); i++)
    {
        if (events[i].type == type)
        {
            outEvent = events[i];
            consumeIdx = (int)i;
            break;
        }
    }

    if (consumeIdx < 0)
        return false;

    events.erase(events.begin() + consumeIdx);
    return true;
}

struct UIWindowState
{
    UIWindow window;
    PoolAllocator widgetStatePA;
    UIWidgetState* state = nullptr; // widget state for the window itself
    Stack<UIWidgetState*> imWidgetStack;

    UIWindowState();
    ~UIWindowState();

    inline UIWidget get_parent_widget()
    {
        return imWidgetStack.empty() ? (UIWidget)window : (UIWidget)imWidgetStack.top()->widget;
    }

    UIWidgetState* get_or_create_widget(UIWidgetType type, void* data);
    UIWidgetState* get_or_create_widget_state(UIWidgetType type);
    UIWidgetState* get_or_create_text_edit(UITextEditData* data);
    UIWidgetState* get_or_create_toggle(UIToggleData* data);
    UIWidgetState* get_or_create_button(UIButtonData* data);
    UIWidgetState* get_or_create_slider(UISliderData* data);
};

struct UIWorkspaceState
{
    UIWorkspace space;
    HashMap<std::string, UIWindowState*> imWindows;

    UIWindowState* get_or_create_window_state(const char* windowName, bool floatingWindow);
    void destroy_window_state(UIWindowState*);
};

UIWindowState* UIWorkspaceState::get_or_create_window_state(const char* windowName, bool floatingWindow)
{
    std::string key(windowName);
    UIWindowState* windowS = nullptr;

    if (imWindows.contains(key))
        return imWindows[key];

    windowS = heap_new<UIWindowState>(MEMORY_USAGE_UI);
    windowS->state = (UIWidgetState*)windowS->widgetStatePA.allocate();
    new (windowS->state) UIWidgetState(UI_WIDGET_WINDOW);

    UILayoutInfo layoutI{};
    layoutI.sizeX = UISize::fit();
    layoutI.sizeY = UISize::fit();
    UIWindowInfo windowI{};
    windowI.name = windowName;
    UIWindow window{};
    if (floatingWindow)
        window = space.create_float_window(layoutI, windowI, windowS->state);
    else
        window = space.create_docked_window(space.get_root_id(), layoutI, windowI, windowS->state);
    window.set_color(0xE0);
    windowS->window = window;
    windowS->state->widget = (UIWidget)window;
    windowS->state->widgetHash = window.get_hash();

    return imWindows[key] = windowS;
}

void UIWorkspaceState::destroy_window_state(UIWindowState* windowS)
{
    heap_delete<UIWindowState>(windowS);
}

struct UILayerState
{
    UILayer layer;
    HashMap<std::string, UIWorkspaceState*> imSpaces;

    UIWorkspaceState* get_or_create_workspace_state(const char* spaceName, const Rect& area);
    void destroy_workspace_state(UIWorkspaceState* spaceS);
};

UIWorkspaceState* UILayerState::get_or_create_workspace_state(const char* spaceName, const Rect& area)
{
    UIWorkspaceState* spaceS = nullptr;

    std::string key(spaceName);
    if (imSpaces.contains(key))
    {
        spaceS = imSpaces[key];
        spaceS->space.set_rect(area);
        return spaceS;
    }

    spaceS = heap_new<UIWorkspaceState>(MEMORY_USAGE_UI);
    spaceS->space = layer.create_workspace(area);
    return imSpaces[key] = spaceS;
}

void UILayerState::destroy_workspace_state(UIWorkspaceState* spaceS)
{
    for (auto it : spaceS->imWindows)
        spaceS->destroy_window_state(it.second);
    spaceS->imWindows.clear();

    layer.destroy_workspace(spaceS->space);

    heap_delete<UIWorkspaceState>(spaceS);
}

struct UIContextState
{
    UIContext ctx{};                              // connected external context
    UILayerState* imLayer = nullptr;              // current layer
    UIWorkspaceState* imSpace = nullptr;          // current workspace
    UIWindowState* imWindow = nullptr;            // current window
    Vector<UIWindowState*> imOverlayWindows;      // overlay window stack
    HashMap<int, std::string> imOverlayUsers;     // overlay window users this frame
    HashMap<std::string, UILayerState*> imLayers; // all layers in this context
    Vec2 screenExtent;

    void set_window_state(UIWindowState* windowS);
    UIWindowState* get_or_create_overlay_window_state(int level);
    UILayerState* get_or_create_layer_state(const char* layerName);
    void destroy_layer_state(UILayerState* layerS);
};

void UIContextState::set_window_state(UIWindowState* windowS)
{
    imWindow = windowS;
    imWindow->state->childCounter = 0;
    imWindow->imWidgetStack.push(imWindow->state);
}

UIWindowState* UIContextState::get_or_create_overlay_window_state(int level)
{
    if (level >= (int)imOverlayWindows.size())
    {
        imOverlayWindows.resize(level + 1);
        for (int i = level; i < (int)imOverlayWindows.size(); i++)
            imOverlayWindows[i] = nullptr;
    }

    if (imOverlayWindows[level])
    {
        (void)ctx.set_overlay_level(level);
        return imOverlayWindows[level];
    }

    UIWindowState* windowS = heap_new<UIWindowState>(MEMORY_USAGE_UI);
    windowS->state = (UIWidgetState*)windowS->widgetStatePA.allocate();
    new (windowS->state) UIWidgetState(UI_WIDGET_WINDOW);

    windowS->window = ctx.set_overlay_level(level);
    windowS->window.set_user(windowS->state);
    windowS->state->widget = (UIWidget)windowS->window;
    windowS->state->widgetHash = windowS->window.get_hash();

    imOverlayWindows[level] = windowS;

    return windowS;
}

UILayerState* UIContextState::get_or_create_layer_state(const char* layerName)
{
    std::string key(layerName);
    if (imLayers.contains(key))
        return imLayers[key];

    UILayerState* layerS = heap_new<UILayerState>(MEMORY_USAGE_UI);
    layerS->layer = ctx.create_layer(layerName);

    return imLayers[key] = layerS;
}

void UIContextState::destroy_layer_state(UILayerState* layerS)
{
    for (auto it : layerS->imSpaces)
        layerS->destroy_workspace_state(it.second);
    layerS->imSpaces.clear();

    ctx.destroy_layer(layerS->layer);

    heap_delete<UILayerState>(layerS);
}

static UIContextState* sImContext = nullptr;              // current ui context
static UIFont sImFont;                                    // default font
static UIFont sImFontMono;                                // default font
static HashMap<std::string, UIContextState*> sImContexts; // all imgui frame contexts

static void on_ui_context_event(UIWidget widget, const UIEvent& event, void* user)
{
    UIContextState* ctxS = (UIContextState*)user;
    UIWidgetState* widgetS = (UIWidgetState*)widget.get_user();

    /*
    if (ctxS->imPopup && ctxS->imPopup != widgetS->window && event.type == UI_EVENT_MOUSE_DOWN)
    {
        ctxS->ctx.clear_overlay_windows();
    }
    */

    // cache events for this frame for imgui API.
    widgetS->events.push_back(event);
}

static UIContextState* get_or_create_context_state(const char* ctxName, const Vec2& screenExtent)
{
    UIContextState* ctxS = nullptr;
    std::string key(ctxName);

    if (sImContexts.contains(key))
    {
        ctxS = sImContexts[key];
        ctxS->screenExtent = screenExtent;
        return ctxS;
    }

    ctxS = sImContexts[key] = heap_new<UIContextState>(MEMORY_USAGE_UI);
    ctxS->screenExtent = screenExtent;

    UIContextInfo ctxI{};
    ctxI.font = sImFont;
    ctxI.fontMono = sImFontMono;
    ctxI.onEvent = &on_ui_context_event;
    ctxI.user = ctxS;
    ctxI.theme = UITheme::get_default_theme();
    ctxS->ctx = UIContext::create(ctxI);

    return ctxS;
}

static void destroy_context_state(UIContextState* ctxS)
{
    for (auto it : ctxS->imLayers)
        ctxS->destroy_layer_state(it.second);
    ctxS->imLayers.clear();

    for (auto windowS : ctxS->imOverlayWindows)
        heap_delete<UIWindowState>(windowS);

    UIContext::destroy(ctxS->ctx);

    heap_delete<UIContextState>(ctxS);
}

static void on_text_change_handler(UIWidget, View text, void* user)
{
    UIWidgetState* widgetS = (UIWidgetState*)user;

    widgetS->textEdit.isChanged.set(true);
    widgetS->textEdit.lastChange = std::string(text.data, text.size);
}

static void on_text_submit_handler(UIWidget, View text, void* user)
{
    UIWidgetState* widgetS = (UIWidgetState*)user;

    widgetS->textEdit.isSubmitted.set(true);
    widgetS->textEdit.lastSubmission = std::string(text.data, text.size);
}

static void destroy_widget_subtree(PoolAllocator statePA, UIWidgetState* widgetS)
{
    for (UIWidgetState* childS : widgetS->children)
    {
        destroy_widget_subtree(statePA, childS);
    }

    widgetS->widget.remove();
    widgetS->~UIWidgetState();
    statePA.free(widgetS);
}

// The state hash consists of the widget type, widget sibling index, and the parent state hash.
// This should be enough to identify uniquely the widget in a tree hierarchy.
static Hash64 get_widget_state_hash(UIWidgetType type, int siblingIndex, Hash64 parentStateHash)
{
    size_t hash64 = (size_t)parentStateHash;

    hash_combine(hash64, type);
    hash_combine(hash64, siblingIndex);

    return (Hash64)hash64;
}

UIWidgetState* UIWindowState::get_or_create_widget(UIWidgetType type, void* data)
{
    UIWidgetState* widgetS = get_or_create_widget_state(type);

    if (widgetS->widget && widgetS->widget.get_type() == type)
    {
        if (data)
            widgetS->widget.set_data(data);
        return widgetS;
    }

    UIWidget parent = get_parent_widget();
    widgetS->widget = parent.add_child(type, UIWidget::get_default_layout(type), data, widgetS);

    return widgetS;
}

// NOTE: has side effect of incrementing the childCounter of top widget
UIWidgetState* UIWindowState::get_or_create_widget_state(UIWidgetType type)
{
    LD_ASSERT(!imWidgetStack.empty());

    UIWidgetState* parentS = imWidgetStack.top();
    Hash64 parentHash = parentS->widgetHash;
    int siblingIndex = parentS->childCounter++;

    if (siblingIndex >= (int)parentS->children.size())
    {
        int oldSize = (int)parentS->children.size();
        parentS->children.resize(siblingIndex + 1);
        for (int i = oldSize; i < siblingIndex + 1; i++)
            parentS->children[i] = nullptr;
    }

    Hash64 widgetHash = get_widget_state_hash(type, siblingIndex, parentHash);
    UIWidgetState* widgetS = parentS->children[siblingIndex];

    if (widgetS)
    {
        if (widgetS->widgetHash == widgetHash)
        {
            LD_ASSERT(widgetS->widget.get_type() == type);

            widgetS->childCounter = 0; // track subtree each frame
            return widgetS;
        }
        else // destroy widget subtrees from siblingIndex onwards
        {
            for (int i = siblingIndex; i < (int)parentS->children.size(); i++)
                destroy_widget_subtree(widgetStatePA, parentS->children[i]);

            LD_ASSERT(parentS->childCounter == siblingIndex + 1);
            parentS->children.resize(parentS->childCounter);
            widgetS = parentS->children[siblingIndex] = nullptr;
        }
    }

    if (!widgetS)
    {
        widgetS = parentS->children[siblingIndex] = (UIWidgetState*)widgetStatePA.allocate();
        new (widgetS) UIWidgetState(type);
        widgetS->widget = {}; // caller creates
        widgetS->widgetHash = widgetHash;
        widgetS->childCounter = 0;
        widgetS->window = this;
    }

    return widgetS;
}

UIWindowState::UIWindowState()
    : state(nullptr)
{
    PoolAllocatorInfo poolAI{};
    poolAI.blockSize = sizeof(UIWidgetState);
    poolAI.isMultiPage = true;
    poolAI.pageSize = 16; // TODO: heuristic of number of widgets in a window?
    poolAI.usage = MEMORY_USAGE_UI;
    widgetStatePA = PoolAllocator::create(poolAI);
}

UIWindowState::~UIWindowState()
{
    for (auto it = widgetStatePA.begin(); it; ++it)
    {
        UIWidgetState* widgetS = (UIWidgetState*)it.data();
        widgetS->~UIWidgetState();
    }

    PoolAllocator::destroy(widgetStatePA);
}

UIWidgetState* UIWindowState::get_or_create_text_edit(UITextEditData* data)
{
    UIWidgetState* widgetS = get_or_create_widget_state(UI_WIDGET_TEXT_EDIT);
    UITextEditWidget textW = (UITextEditWidget)widgetS->widget;

    if (widgetS->widget && widgetS->widget.get_type() == UI_WIDGET_TEXT_EDIT)
    {
        if (data)
            textW.set_data(data);
        return widgetS;
    }

    UIWidget parent = get_parent_widget();
    widgetS->widget = parent.add_child(UI_WIDGET_TEXT_EDIT, {}, data, widgetS);
    widgetS->widget.set_layout_size(UISize::fixed(100.0f), UISize::fixed(19.2f)); // TODO:

    textW = (UITextEditWidget)widgetS->widget;
    textW.set_on_submit(&on_text_submit_handler);
    textW.set_on_change(&on_text_change_handler);

    return widgetS;
}

UIWidgetState* UIWindowState::get_or_create_toggle(UIToggleData* data)
{
    UIWidgetState* widgetS = get_or_create_widget_state(UI_WIDGET_TOGGLE);
    UIToggleWidget toggleW = (UIToggleWidget)widgetS->widget;

    if (widgetS->widget && widgetS->widget.get_type() == UI_WIDGET_TOGGLE)
    {
        if (data)
            toggleW.set_data(data);
        return widgetS;
    }

    UILayoutInfo layoutI{};
    layoutI.sizeX = UISize::grow();
    layoutI.sizeY = UISize::fit();

    data->state = false;

    UIWidget parent = get_parent_widget();
    widgetS->widget = parent.add_child(UI_WIDGET_TOGGLE, layoutI, data, widgetS);

    toggleW = (UIToggleWidget)widgetS->widget;
    toggleW.set_on_toggle([](UIWidget, bool, void* user) {
        UIWidgetState* widgetS = (UIWidgetState*)user;
        widgetS->isTogglePressed.set(true);
    });

    return widgetS;
}

UIWidgetState* UIWindowState::get_or_create_button(UIButtonData* data)
{
    UIWidgetState* widgetS = get_or_create_widget_state(UI_WIDGET_BUTTON);
    UIButtonWidget buttonW = (UIButtonWidget)widgetS->widget;

    if (widgetS->widget && widgetS->widget.get_type() == UI_WIDGET_BUTTON)
    {
        if (data)
            buttonW.set_data(data);
        return widgetS;
    }

    UILayoutInfo layoutI{};
    layoutI.sizeX = UISize::fixed(100.0f);
    layoutI.sizeY = UISize::fixed(20.0f);

    UIWidget parent = get_parent_widget();
    widgetS->widget = parent.add_child(UI_WIDGET_BUTTON, layoutI, data, widgetS);

    buttonW = (UIButtonWidget)widgetS->widget;
    buttonW.set_on_click([](UIWidget, MouseButton, void* user) {
        auto* state = (UIWidgetState*)user;
        state->isButtonPressed.set(true);
    });

    return widgetS;
}

UIWidgetState* UIWindowState::get_or_create_slider(UISliderData* data)
{
    UIWidgetState* widgetS = get_or_create_widget_state(UI_WIDGET_SLIDER);
    UISliderWidget sliderW = (UISliderWidget)widgetS->widget;

    if (widgetS->widget && widgetS->widget.get_type() == UI_WIDGET_SLIDER)
    {
        if (data)
            sliderW.set_data(data);
        return widgetS;
    }

    UILayoutInfo layoutI{};
    layoutI.sizeX = UISize::grow();
    layoutI.sizeY = UISize::fixed(20.0f);

    UIWidget parent = get_parent_widget();
    widgetS->widget = parent.add_child(UI_WIDGET_SLIDER, layoutI, data, widgetS);

    return widgetS;
}

void ui_imgui_startup(UIFont font, UIFont fontMono)
{
    LD_ASSERT(!sImContext);

    sImFont = font;
    sImFontMono = fontMono;
}

void ui_imgui_cleanup()
{
    LD_ASSERT_NOT_UI_CONTEXT_SCOPE;

    for (const auto& it : sImContexts)
        destroy_context_state(it.second);
    sImContexts.clear();
}

void ui_imgui_cleanup_context(const char* ctxName)
{
    LD_ASSERT_NOT_UI_CONTEXT_SCOPE;

    auto it = sImContexts.find(ctxName);

    if (it != sImContexts.end())
    {
        destroy_context_state(it->second);
        sImContexts.erase(it);
    }
}

void ui_imgui_cleanup_layer(const char* ctxName, const char* layerName)
{
    LD_ASSERT_NOT_UI_CONTEXT_SCOPE;

    if (!sImContexts.contains(ctxName))
        return;

    UIContextState* ctxS = sImContexts[ctxName];
    if (!ctxS->imLayers.contains(layerName))
        return;

    UILayerState* layerS = ctxS->imLayers[layerName];
    ctxS->destroy_layer_state(layerS);
    ctxS->imLayers.erase(layerName);
}

void ui_imgui_cleanup_workspace(const char* ctxName, const char* layerName, const char* workspaceName)
{
    LD_ASSERT_NOT_UI_CONTEXT_SCOPE;

    if (!sImContexts.contains(ctxName))
        return;

    UIContextState* ctxS = sImContexts[ctxName];
    if (!ctxS->imLayers.contains(layerName))
        return;

    UILayerState* layerS = ctxS->imLayers[layerName];
    if (!layerS->imSpaces.contains(workspaceName))
        return;

    UIWorkspaceState* spaceS = layerS->imSpaces[workspaceName];
    layerS->destroy_workspace_state(spaceS);
    layerS->imSpaces.erase(workspaceName);
}

void ui_imgui_set_layer_visible(const char* ctxName, const char* layerName, bool isVisible)
{
    LD_ASSERT_NOT_UI_CONTEXT_SCOPE;

    if (!sImContexts.contains(ctxName))
        return;

    UIContextState* ctxS = sImContexts[ctxName];
    if (!ctxS->imLayers.contains(layerName))
        return;

    UILayerState* layerS = ctxS->imLayers[layerName];
    layerS->layer.set_visible(isVisible);
}

bool ui_context_input(const char* ctxName, const WindowEvent* event)
{
    Vec2 screenExtent(0.0f);
    std::string key(ctxName);

    if (sImContexts.contains(key))
        screenExtent = sImContexts[key]->screenExtent;

    UIContextState* ctxS = get_or_create_context_state(ctxName, screenExtent);
    return ctxS->ctx.input_window_event(event);
}

void ui_context_render(const char* ctxName, ScreenRenderComponent renderer)
{
    Vec2 screenExtent(0.0f);
    std::string key(ctxName);

    if (sImContexts.contains(key))
        screenExtent = sImContexts[key]->screenExtent;

    UIContextState* ctxS = get_or_create_context_state(ctxName, screenExtent);
    ctxS->ctx.render(renderer);
}

void ui_context_begin(const char* ctxName, const Vec2& screenExtent)
{
    LD_ASSERT(!sImContext && "ui_frame_end not called");

    sImContext = get_or_create_context_state(ctxName, screenExtent);
}

void ui_context_end(float delta, CursorType& outCursorHint)
{
    LD_ASSERT_UI_CONTEXT_SCOPE;
    LD_ASSERT(!sImContext->imWindow && "ui_pop_window not called");

    // sImContext->popupLayer->layer.raise(); // always on top of user declared layers
    sImContext->ctx.update(sImContext->screenExtent, delta);

    outCursorHint = sImContext->ctx.get_cursor_hint();
    sImContext = nullptr;
}

void ui_layer_begin(const char* layerName)
{
    LD_ASSERT_UI_CONTEXT_SCOPE;

    sImContext->imLayer = sImContext->get_or_create_layer_state(layerName);
    sImContext->imLayer->layer.raise();
}

void ui_layer_set_visible(bool isVisible)
{
    LD_ASSERT_UI_LAYER_SCOPE;

    sImContext->imLayer->layer.set_visible(isVisible);
}

void ui_layer_end()
{
    LD_ASSERT_UI_LAYER_SCOPE;

    sImContext->imLayer = nullptr;
}

void ui_workspace_begin(const char* workspaceName, const Rect& area)
{
    LD_ASSERT_UI_LAYER_SCOPE;

    sImContext->imSpace = sImContext->imLayer->get_or_create_workspace_state(workspaceName, area);
    sImContext->imSpace->space.raise();
}

void ui_workspace_end()
{
    LD_ASSERT_UI_LAYER_SCOPE;

    sImContext->imSpace = nullptr;
}

void ui_top_layout(const UILayoutInfo& layoutI)
{
    LD_ASSERT_UI_TOP_WIDGET;

    UIWidgetState* widgetS = sImContext->imWindow->imWidgetStack.top();
    widgetS->widget.set_layout(layoutI);
}

void ui_top_layout_size(UISize sizeX, UISize sizeY)
{
    LD_ASSERT_UI_TOP_WIDGET;

    UIWidgetState* widgetS = sImContext->imWindow->imWidgetStack.top();
    widgetS->widget.set_layout_size(sizeX, sizeY);
}

void ui_top_layout_size_x(UISize sizeX)
{
    LD_ASSERT_UI_TOP_WIDGET;

    UIWidgetState* widgetS = sImContext->imWindow->imWidgetStack.top();
    widgetS->widget.set_layout_size_x(sizeX);
}

void ui_top_layout_size_y(UISize sizeY)
{
    LD_ASSERT_UI_TOP_WIDGET;

    UIWidgetState* widgetS = sImContext->imWindow->imWidgetStack.top();
    widgetS->widget.set_layout_size_y(sizeY);
}

void ui_top_layout_child_axis(UIAxis childAxis)
{
    LD_ASSERT_UI_TOP_WIDGET;

    UIWidgetState* widgetS = sImContext->imWindow->imWidgetStack.top();
    widgetS->widget.set_layout_child_axis(childAxis);
}

void ui_top_layout_child_padding(const UIPadding& pad)
{
    LD_ASSERT_UI_TOP_WIDGET;

    UIWidgetState* widgetS = sImContext->imWindow->imWidgetStack.top();
    widgetS->widget.set_layout_child_padding(pad);
}

void ui_top_layout_child_gap(float childGap)
{
    LD_ASSERT_UI_TOP_WIDGET;

    UIWidgetState* widgetS = sImContext->imWindow->imWidgetStack.top();
    widgetS->widget.set_layout_child_gap(childGap);
}

void ui_top_user(void* imUser)
{
    LD_ASSERT_UI_TOP_WIDGET;

    UIWidgetState* widgetS = sImContext->imWindow->imWidgetStack.top();
    widgetS->imUser = imUser;
}

void ui_top_get_rect(Rect& outRect)
{
    LD_ASSERT_UI_TOP_WIDGET;

    UIWidgetState* widgetS = sImContext->imWindow->imWidgetStack.top();
    outRect = widgetS->widget.get_rect();
}

bool ui_top_get_mouse_pos(Vec2& outMousePos)
{
    LD_ASSERT_UI_TOP_WIDGET;

    UIWidgetState* widgetS = sImContext->imWindow->imWidgetStack.top();
    return widgetS->widget.get_mouse_pos(outMousePos);
}

void ui_top_draw(const IMDrawCallback& imDrawCallback)
{
    LD_ASSERT_UI_TOP_WIDGET;

    UIWidgetState* widgetS = sImContext->imWindow->imWidgetStack.top();
    widgetS->onDraw = imDrawCallback;
    widgetS->widget.set_on_draw([](UIWidget widget, ScreenRenderComponent renderer) {
        UIWidgetState* widgetS = (UIWidgetState*)widget.get_user();
        widgetS->onDraw(widgetS->widget, renderer, widgetS->imUser);
    });
}

bool ui_top_drag(MouseButton& dragBtn, Vec2& dragPos, bool& dragBegin)
{
    LD_ASSERT_UI_TOP_WIDGET;

    UIWidgetState* widgetS = sImContext->imWindow->imWidgetStack.top();

    if (!widgetS->widget.has_on_event())
        widgetS->widget.set_consume_mouse_event(true);

    UIEvent event;
    if (widgetS->consume_event(UI_EVENT_MOUSE_DRAG, event))
    {
        dragBtn = event.drag.button;
        dragPos = event.drag.position;
        dragBegin = event.drag.begin;
        return true;
    }

    return false;
}

bool ui_top_is_dragged()
{
    LD_ASSERT_UI_TOP_WIDGET;

    UIWidgetState* widgetS = sImContext->imWindow->imWidgetStack.top();

    return widgetS->widget.is_dragged();
}

bool ui_top_hover(UIEventType& outHover)
{
    LD_ASSERT_UI_TOP_WIDGET;

    UIWidgetState* widgetS = sImContext->imWindow->imWidgetStack.top();

    if (!widgetS->widget.has_on_event())
        widgetS->widget.set_consume_mouse_event(true);

    UIEvent event;
    if (widgetS->consume_event(UI_EVENT_MOUSE_ENTER, event))
    {
        outHover = event.type;
        return true;
    }

    if (widgetS->consume_event(UI_EVENT_MOUSE_LEAVE, event))
    {
        outHover = event.type;
        return true;
    }

    return false;
}

bool ui_top_is_hovered()
{
    LD_ASSERT_UI_TOP_WIDGET;

    UIWidgetState* widgetS = sImContext->imWindow->imWidgetStack.top();

    return widgetS->widget.is_hovered();
}

bool ui_top_scroll(Vec2& scroll)
{
    LD_ASSERT_UI_TOP_WIDGET;

    UIWidgetState* widgetS = sImContext->imWindow->imWidgetStack.top();

    if (!widgetS->widget.has_on_event())
        widgetS->widget.set_consume_scroll_event(true);

    UIEvent event;
    if (widgetS->consume_event(UI_EVENT_SCROLL, event))
    {
        scroll = event.scroll.offset;
        return true;
    }

    return false;
}

bool ui_top_mouse_down(MouseValue& outMouse, Vec2& outMousePos)
{
    LD_ASSERT_UI_TOP_WIDGET;

    UIWidgetState* widgetS = sImContext->imWindow->imWidgetStack.top();

    if (!widgetS->widget.has_on_event())
        widgetS->widget.set_consume_mouse_event(true);

    UIEvent event;
    if (widgetS->consume_event(UI_EVENT_MOUSE_DOWN, event))
    {
        outMouse = MouseValue(event.mouse.button, event.mouse.mods);
        outMousePos = event.mouse.position + widgetS->widget.get_pos();
        return true;
    }

    return false;
}

bool ui_top_mouse_up(MouseValue& outMouse, Vec2& outMousePos)
{
    LD_ASSERT_UI_TOP_WIDGET;

    UIWidgetState* widgetS = sImContext->imWindow->imWidgetStack.top();

    if (!widgetS->widget.has_on_event())
        widgetS->widget.set_consume_mouse_event(true);

    UIEvent event;
    if (widgetS->consume_event(UI_EVENT_MOUSE_UP, event))
    {
        outMouse = MouseValue(event.mouse.button, event.mouse.mods);
        outMousePos = event.mouse.position + widgetS->widget.get_pos();
        return true;
    }

    return false;
}

bool ui_top_key_down(KeyValue& outKey)
{
    LD_ASSERT_UI_TOP_WIDGET;

    UIWidgetState* widgetS = sImContext->imWindow->imWidgetStack.top();

    if (!widgetS->widget.has_on_event())
        widgetS->widget.set_consume_key_event(true);

    UIEvent event;
    if (widgetS->consume_event(UI_EVENT_KEY_DOWN, event))
    {
        outKey = KeyValue(event.key.code, event.key.mods);
        return true;
    }

    return false;
}

bool ui_top_key_up(KeyValue& outKey)
{
    LD_ASSERT_UI_TOP_WIDGET;

    UIWidgetState* widgetS = sImContext->imWindow->imWidgetStack.top();

    if (!widgetS->widget.has_on_event())
        widgetS->widget.set_consume_key_event(true);

    UIEvent event;
    if (widgetS->consume_event(UI_EVENT_KEY_UP, event))
    {
        outKey = KeyValue(event.key.code, event.key.mods);
        return true;
    }

    return false;
}

void ui_pop()
{
    LD_ASSERT(sImContext->imWindow && "missing window");
    LD_ASSERT(!sImContext->imWindow->imWidgetStack.empty() && "widget stack empty");

    UIWindowState* windowS = sImContext->imWindow;
    UIWidgetState* parentS = windowS->imWidgetStack.top();

    // make sure events don't bleed into next frame
    parentS->events.clear();

    switch (parentS->type)
    {
    case UI_WIDGET_TEXT:
        parentS->text.spanClicks.clear();
        break;
    default:
        break;
    }

    for (int i = parentS->childCounter; i < (int)parentS->children.size(); i++)
    {
        // Widget tree hierarchy has changed compared to previous frame.
        // Trim excessive subtrees.
        destroy_widget_subtree(windowS->widgetStatePA, parentS->children[i]);
    }
    parentS->children.resize(parentS->childCounter);

    sImContext->imWindow->imWidgetStack.pop();
}

void ui_pop_window()
{
    LD_ASSERT(sImContext->imWindow && "missing window");
    LD_ASSERT(sImContext->imWindow->imWidgetStack.size() == 1 && "some widget pushed but not popped");

    // last widget state on stack is the window
    ui_pop();

    sImContext->imWindow = nullptr;
}

void ui_push_window(const char* windowName)
{
    LD_ASSERT_UI_WORKSPACE_SCOPE;

    // docked window within workspace
    UIWindowState* windowS = sImContext->imSpace->get_or_create_window_state(windowName, false);

    sImContext->set_window_state(windowS);
}

void ui_window_set_color(Color color)
{
    LD_ASSERT_UI_WINDOW_SCOPE;

    sImContext->imWindow->window.set_color(color);
}

void ui_window_set_rect(const Rect& rect)
{
    LD_ASSERT_UI_WINDOW_SCOPE;

    sImContext->imWindow->window.set_rect(rect);
}

void ui_request_overlay_window(const char* overlayUser, int level, Vec2 position)
{
    LD_ASSERT_UI_WINDOW_SCOPE;

    sImContext->imOverlayUsers[level] = overlayUser;

    UIWindowState* popupS = sImContext->get_or_create_overlay_window_state(level);
    popupS->window.set_pos(position);
}

void ui_clear_overlay_windows()
{
    LD_ASSERT_UI_CONTEXT_SCOPE;

    sImContext->ctx.clear_overlay_windows();
    sImContext->imOverlayUsers.clear();
}

bool ui_push_overlay_window(const char* overlayUser)
{
    LD_ASSERT_UI_CONTEXT_SCOPE;

    int level = -1;

    for (const auto it : sImContext->imOverlayUsers)
    {
        if (it.second == overlayUser)
        {
            level = it.first;
            break;
        }
    }

    if (level < 0 || level > sImContext->ctx.get_overlay_level())
        return false;

    UIWindowState* windowS = sImContext->imOverlayWindows[level];
    if (!windowS)
        return false;

    sImContext->set_window_state(windowS);

    return true;
}

UITextWidget ui_push_text(UITextData* data)
{
    LD_ASSERT_UI_PUSH;

    UIWindowState* imWindow = sImContext->imWindow;
    UIWidgetState* imWidget = imWindow->get_or_create_widget(UI_WIDGET_TEXT, data);
    UITextWidget textW = (UITextWidget)imWidget->widget;
    LD_ASSERT(textW.get_type() == UI_WIDGET_TEXT);

    imWindow->imWidgetStack.push(imWidget);
    data = (UITextData*)textW.get_data();

    // intercept all span callbacks for imgui API,
    // this is done per-frame as user may modify
    // UITextdata any time.
    data->set_span_on_event(&UITextState::on_span_event, imWidget);

    return textW;
}

UITextWidget ui_push_text(UITextData* data, const char* text)
{
    UITextWidget textW = ui_push_text(data);
    data = (UITextData*)textW.get_data();

    if (text)
        data->set_value(std::string(text));
    else
        data->clear_value();

    return textW;
}

void ui_text_style(Color color, TextSpanFont font)
{
    LD_ASSERT_UI_TOP_WIDGET_TYPE(UI_WIDGET_TEXT);

    UIWidgetState* imWidget = sImContext->imWindow->imWidgetStack.top();
    UITextWidget textW = (UITextWidget)imWidget->widget;

    textW.set_text_style(color, font);
}

bool ui_text_span_hovered(int index)
{
    LD_ASSERT_UI_TOP_WIDGET_TYPE(UI_WIDGET_TEXT);

    if (index < 0)
        return false;

    UIWidgetState* imWidget = sImContext->imWindow->imWidgetStack.top();
    UITextWidget textW = (UITextWidget)imWidget->widget;

    return index == textW.get_span_index();
}

bool ui_text_span_pressed(int spanIndex)
{
    LD_ASSERT_UI_TOP_WIDGET_TYPE(UI_WIDGET_TEXT);

    UIWidgetState* imWidget = sImContext->imWindow->imWidgetStack.top();

    const Vector<int>& clicks = imWidget->text.spanClicks;

    return std::find(clicks.begin(), clicks.end(), spanIndex) != clicks.end();
}

bool UITextState::on_span_event(UIWidget widget, const UIEvent& event, UITextSpan& span, int spanIndex, void* user)
{
    UIWidgetState* imWidget = (UIWidgetState*)user;
    LD_ASSERT(imWidget && imWidget->type == UI_WIDGET_TEXT);

    if (event.type == UI_EVENT_MOUSE_DOWN)
        imWidget->text.spanClicks.push_back(spanIndex);

    return true;
}

UITextEditWidget ui_push_text_edit(UITextEditData* data)
{
    LD_ASSERT_UI_PUSH;

    UIWindowState* imWindow = sImContext->imWindow;
    UIWidgetState* imWidget = imWindow->get_or_create_text_edit(data);
    UITextEditWidget textW = (UITextEditWidget)imWidget->widget;
    LD_ASSERT(textW.get_type() == UI_WIDGET_TEXT_EDIT);

    imWindow->imWidgetStack.push(imWidget);

    return textW;
}

bool ui_text_edit_is_editing()
{
    LD_ASSERT_UI_TOP_WIDGET_TYPE(UI_WIDGET_TEXT_EDIT);

    UIWidgetState* imWidget = sImContext->imWindow->imWidgetStack.top();
    UITextEditWidget textW = (UITextEditWidget)imWidget->widget;

    return textW.is_editing();
}

bool ui_text_edit_changed(std::string& text)
{
    LD_ASSERT_UI_TOP_WIDGET_TYPE(UI_WIDGET_TEXT_EDIT);

    UIWidgetState* imWidget = sImContext->imWindow->imWidgetStack.top();

    if (imWidget->textEdit.isChanged.read())
    {
        text = imWidget->textEdit.lastChange;
        imWidget->textEdit.lastChange.clear();
        return true;
    }

    return false;
}

bool ui_text_edit_submitted(std::string& text)
{
    LD_ASSERT_UI_TOP_WIDGET_TYPE(UI_WIDGET_TEXT_EDIT);

    UIWidgetState* imWidget = sImContext->imWindow->imWidgetStack.top();

    if (imWidget->textEdit.isSubmitted.read())
    {
        text = imWidget->textEdit.lastSubmission;
        imWidget->textEdit.lastSubmission.clear();
        return true;
    }

    return false;
}

UIDropdownWidget ui_push_dropdown(UIDropdownData* data)
{
    LD_ASSERT_UI_PUSH;

    UIWindowState* imWindow = sImContext->imWindow;
    UIWidgetState* imWidget = imWindow->get_or_create_widget(UI_WIDGET_DROPDOWN, data);
    UIDropdownWidget dropdownW = (UIDropdownWidget)imWidget->widget;
    UIDropdownData* dropdownData = (UIDropdownData*)dropdownW.get_data();
    dropdownData->onOpen = [](UIWidget widget) {
        auto* state = (UIWidgetState*)widget.get_user();
        state->dropdown.isOpened.set(true);
    };

    imWindow->imWidgetStack.push(imWidget);

    return dropdownW;
}

bool ui_dropdown_is_opened()
{
    LD_ASSERT_UI_TOP_WIDGET_TYPE(UI_WIDGET_DROPDOWN);

    UIWidgetState* imWidget = sImContext->imWindow->imWidgetStack.top();

    return imWidget->dropdown.isOpened.read();
}

void ui_dropdown_overlay(UIDropdownWidget widget)
{
    LD_ASSERT_UI_WINDOW_SCOPE;

    if (!widget)
        return;

    UIDropdownData* data = (UIDropdownData*)widget.get_data();

    ui_push_panel(nullptr, 0x101010FF);
    ui_top_layout(UILayoutInfo(UISize::fit(), UISize::fit(), UI_AXIS_Y));

    for (size_t i = 0; i < data->options.size(); i++)
    {
        const std::string& opt = data->options[i];
        MouseValue mouseVal;
        Vec2 mousePos;

        UITextData* textData = (UITextData*)ui_push_text(nullptr, opt.c_str()).get_data();
        if (ui_top_mouse_down(mouseVal, mousePos) && mouseVal.button() == MOUSE_BUTTON_LEFT)
        {
            widget.set_option(i);
            ui_clear_overlay_windows();
        }
        ui_pop();
    }

    ui_pop();
}

UIImageWidget ui_push_image(UIImageData* data, float width, float height)
{
    LD_ASSERT_UI_PUSH;

    UIWindowState* imWindow = sImContext->imWindow;
    UIWidgetState* imWidget = imWindow->get_or_create_widget(UI_WIDGET_IMAGE, data);
    UIImageWidget imageW = (UIImageWidget)imWidget->widget;

    imageW.set_layout_size(UISize::fixed(width), UISize::fixed(height));

    imWindow->imWidgetStack.push(imWidget);

    return imageW;
}

UIPanelWidget ui_push_panel(UIPanelData* data)
{
    LD_ASSERT_UI_PUSH;

    UIWindowState* imWindow = sImContext->imWindow;
    UIWidgetState* imWidget = imWindow->get_or_create_widget(UI_WIDGET_PANEL, data);

    imWindow->imWidgetStack.push(imWidget);

    return (UIPanelWidget)imWidget->widget;
}

UIPanelWidget ui_push_panel(UIPanelData* data, Color color)
{
    UIPanelWidget panelW = ui_push_panel(data);
    data = (UIPanelData*)panelW.get_data();
    data->color = color;

    return panelW;
}

UIToggleWidget ui_push_toggle(UIToggleData* data)
{
    LD_ASSERT_UI_PUSH;

    UIWindowState* imWindow = sImContext->imWindow;
    UIWidgetState* imWidget = imWindow->get_or_create_toggle(data);
    UIToggleWidget toggleW = (UIToggleWidget)imWidget->widget;
    LD_ASSERT(toggleW.get_type() == UI_WIDGET_TOGGLE);

    imWindow->imWidgetStack.push(imWidget);

    return toggleW;
}

bool ui_toggle_is_pressed()
{
    LD_ASSERT_UI_TOP_WIDGET_TYPE(UI_WIDGET_TOGGLE);

    UIWidgetState* imWidget = sImContext->imWindow->imWidgetStack.top();

    return imWidget->isTogglePressed.read();
}

UIScrollWidget ui_push_scroll(UIScrollData* data)
{
    LD_ASSERT_UI_PUSH;

    UIWindowState* imWindow = sImContext->imWindow;
    UIWidgetState* imWidget = imWindow->get_or_create_widget(UI_WIDGET_SCROLL, data);

    imWindow->imWidgetStack.push(imWidget);

    return (UIScrollWidget)imWidget->widget;
}

UIScrollBarWidget ui_push_scroll_bar(UIScrollBarData* data)
{
    LD_ASSERT_UI_PUSH;

    UIWindowState* imWindow = sImContext->imWindow;
    UIWidgetState* imWidget = imWindow->get_or_create_widget(UI_WIDGET_SCROLL_BAR, data);

    imWindow->imWidgetStack.push(imWidget);

    return (UIScrollBarWidget)imWidget->widget;
}

UIButtonWidget ui_push_button(UIButtonData* data)
{
    LD_ASSERT_UI_PUSH;

    UIWindowState* imWindow = sImContext->imWindow;
    UIWidgetState* imWidget = imWindow->get_or_create_button(data);

    imWindow->imWidgetStack.push(imWidget);

    return (UIButtonWidget)imWidget->widget;
}

UIButtonWidget ui_push_button(UIButtonData* data, const char* text)
{
    LD_ASSERT_UI_PUSH;

    UIButtonWidget btnW = ui_push_button(data);
    data = (UIButtonData*)btnW.get_data();

    if (text)
        data->text = std::string(text);
    else
        data->text.clear();

    return btnW;
}

bool ui_button_is_pressed()
{
    LD_ASSERT_UI_TOP_WIDGET_TYPE(UI_WIDGET_BUTTON);

    UIWidgetState* imWidget = sImContext->imWindow->imWidgetStack.top();

    return imWidget->isButtonPressed.read();
}

UISliderWidget ui_push_slider(UISliderData* data, float* value)
{
    LD_ASSERT_UI_PUSH;

    UIWindowState* imWindow = sImContext->imWindow;
    UIWidgetState* imWidget = imWindow->get_or_create_slider(data);
    UISliderWidget sliderW = (UISliderWidget)imWidget->widget;
    LD_ASSERT(sliderW.get_type() == UI_WIDGET_SLIDER);

    if (value)
        *value = sliderW.get_value();

    imWindow->imWidgetStack.push(imWidget);

    return sliderW;
}

} // namespace LD
