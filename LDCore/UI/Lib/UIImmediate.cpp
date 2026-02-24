#include <Ludens/DSA/HashMap.h>
#include <Ludens/DSA/Stack.h>
#include <Ludens/DSA/Vector.h>
#include <Ludens/Header/Assert.h>
#include <Ludens/Header/Hash.h>
#include <Ludens/Header/Impulse.h>
#include <Ludens/Header/KeyValue.h>
#include <Ludens/Header/Types.h>
#include <Ludens/Memory/Allocator.h>
#include <Ludens/Profiler/Profiler.h>
#include <Ludens/UI/UIImmediate.h>

#define LD_ASSERT_UI_FRAME_BEGIN LD_ASSERT(sImFrame && "ui_frame_begin not called")
#define LD_ASSERT_UI_WINDOW LD_ASSERT(sImFrame->imWindow && "ui_push_window(_client) not called")

#define LD_ASSERT_UI_PUSH_WINDOW \
    LD_ASSERT_UI_FRAME_BEGIN;    \
    LD_ASSERT(!sImFrame->imWindow && "ui_push_window already called");

#define LD_ASSERT_UI_PUSH LD_ASSERT_UI_WINDOW

#define LD_ASSERT_UI_TOP_WIDGET \
    LD_ASSERT_UI_FRAME_BEGIN;   \
    LD_ASSERT_UI_WINDOW

#define LD_ASSERT_UI_TOP_WIDGET_TYPE(TYPE) \
    LD_ASSERT_UI_TOP_WIDGET;               \
    LD_ASSERT(sImFrame->imWindow->imWidgetStack.top()->type == (TYPE))

namespace LD {

struct UIWindowState;

struct UITextEditState
{
    std::string lastChange;
    std::string lastSubmission;
    Impulse isChanged;
    Impulse isSubmitted;
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
        UITextEditState textEdit;
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
    case UI_WIDGET_TEXT_EDIT:
        new (&textEdit) UITextEditState();
        break;
    default:
        break;
    }
}

UIWidgetState::~UIWidgetState()
{
    switch (type)
    {
    case UI_WIDGET_TEXT_EDIT:
        (&textEdit)->~UITextEditState();
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
    UIWidgetState* state; // widget state for the window itself
    Stack<UIWidgetState*> imWidgetStack;
    Hash64 windowHash;

    UIWindowState();
    ~UIWindowState();

    inline UIWidget get_parent_widget()
    {
        return imWidgetStack.empty() ? (UIWidget)window : (UIWidget)imWidgetStack.top()->widget;
    }

    UIWidgetState* get_or_create_widget_state(UIWidgetType type);
    UIWidgetState* get_or_create_text();
    UIWidgetState* get_or_create_text_edit();
    UIWidgetState* get_or_create_image(RImage image);
    UIWidgetState* get_or_create_panel();
    UIWidgetState* get_or_create_toggle();
    UIWidgetState* get_or_create_scroll(Color bgColor);
    UIWidgetState* get_or_create_button(const char* text);
    UIWidgetState* get_or_create_slider();
};

/// @brief Imgui resources for a single UIContext.
struct UIImmediateFrame
{
    UIContext ctx;                             // connected external context
    UIWindowState* imWindow = nullptr;         // current window
    UIWindowState* imPopup = nullptr;          // global popup window
    HashMap<Hash64, UIWindowState*> imWindows; // all window states
    HashMap<std::string, UIWindow> imPopups;   // all popup windows, owned by imgui
    UILayer popupLayer{};
    UIWorkspace popupWorkspace{};

    void clear_popup_window();
    void set_window_state(UIWindowState* windowS);
    UIWindow get_or_create_popup_window(const char* popupName);
};

void UIImmediateFrame::clear_popup_window()
{
    if (imPopup)
    {
        imPopup->window.hide();
        imPopup = nullptr;
    }
}

void UIImmediateFrame::set_window_state(UIWindowState* windowS)
{
    imWindow = windowS;
    imWindow->state->childCounter = 0;
    imWindow->imWidgetStack.push(imWindow->state);
}

UIWindow UIImmediateFrame::get_or_create_popup_window(const char* popupName)
{
    std::string key(popupName);
    if (imPopups.contains(key))
        return imPopups[key];

    UILayoutInfo layoutI{};
    layoutI.sizeX = UISize::fit();
    layoutI.sizeY = UISize::fit();
    UIWindowInfo windowI{};
    windowI.name = popupName;
    UIWindow popupW = popupWorkspace.create_float_window(layoutI, windowI, nullptr);
    imPopups[key] = popupW;

    return popupW;
}

static UIImmediateFrame* sImFrame = nullptr;                // current imgui frame context
static HashMap<UIContextObj*, UIImmediateFrame*> sImFrames; // all imgui frame contexts

static UIImmediateFrame* get_or_create_immediate_frame(UIContext ctx, const Vec2& screenExtent)
{
    // based on address stability, this is enough for hash key.
    UIContextObj* obj = ctx.unwrap();
    UIImmediateFrame* imFrame = nullptr;
    const Rect screenRect(0.0f, 0.0f, screenExtent.x, screenExtent.y);

    if (sImFrames.contains(obj))
    {
        imFrame = sImFrames[obj];
        imFrame->popupWorkspace.set_rect(screenRect);
        return sImFrames[obj];
    }

    imFrame = sImFrames[obj] = heap_new<UIImmediateFrame>(MEMORY_USAGE_UI);
    imFrame->ctx = ctx;
    imFrame->popupLayer = ctx.create_layer("ImguiPopups");
    imFrame->popupWorkspace = imFrame->popupLayer.create_workspace(screenRect);

    return imFrame;
}

static void on_ui_context_event(UIWidget widget, const UIEvent& event, void* user)
{
    UIImmediateFrame* frame = (UIImmediateFrame*)user;
    UIWidgetState* widgetS = (UIWidgetState*)widget.get_user();

    if (frame->imPopup && frame->imPopup != widgetS->window && event.type == UI_EVENT_MOUSE_DOWN)
    {
        frame->clear_popup_window();
    }

    // cache events for this frame for imgui API.
    widgetS->events.push_back(event);
}

static void on_text_change_handler(UITextEditWidget widget, View text, void* user)
{
    UIWidgetState* widgetS = (UIWidgetState*)user;

    widgetS->textEdit.isChanged.set(true);
    widgetS->textEdit.lastChange = std::string(text.data, text.size);
}

static void on_text_submit_handler(UITextEditWidget widget, View text, void* user)
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

    widgetS->widget.node().remove();
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

static UIWindowState* get_or_create_window_state(UIWindow client)
{
    Hash64 windowHash = client.get_hash();
    UIWindowState* windowS = nullptr;

    if (sImFrame->imWindows.contains(windowHash))
    {
        windowS = sImFrame->imWindows[windowHash];
        LD_ASSERT(windowS->window.unwrap() == client.unwrap());
        return windowS;
    }

    windowS = heap_new<UIWindowState>(MEMORY_USAGE_UI);
    windowS->windowHash = windowHash;
    windowS->window = client;
    windowS->state = (UIWidgetState*)windowS->widgetStatePA.allocate();
    new (windowS->state) UIWidgetState(UI_WIDGET_WINDOW);
    windowS->state->widget = (UIWidget)client;
    windowS->state->widgetHash = windowS->windowHash;

    sImFrame->imWindows[windowHash] = windowS;

    // the imgui layer becomes the user of all windows or widgets
    void* ptr = client.get_user();
    LD_ASSERT(!ptr || ptr == windowS->state);
    client.set_user(windowS->state);

    return windowS;
}

// NOTE: modifies sImWindows, caller should not iterate upon
static void destroy_window_state(UIWindowState* state)
{
    LD_ASSERT(sImFrame && state->window);

    if (!sImFrame->imWindows.contains(state->windowHash))
        return;

    sImFrame->imWindows.erase(state->windowHash);
    heap_delete<UIWindowState>(state);
}

UIWindowState::UIWindowState()
    : state(nullptr), windowHash{}
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
    for (auto ite = widgetStatePA.begin(); ite; ++ite)
    {
        UIWidgetState* widgetS = (UIWidgetState*)ite.data();
        widgetS->~UIWidgetState();
    }

    PoolAllocator::destroy(widgetStatePA);
}

UIWidgetState* UIWindowState::get_or_create_text()
{
    UIWidgetState* widgetS = get_or_create_widget_state(UI_WIDGET_TEXT);

    if (widgetS->widget && widgetS->widget.get_type() == UI_WIDGET_TEXT)
        return widgetS;

    UITextWidgetInfo textWI{};
    textWI.cstr = nullptr;
    textWI.fontSize = 16.0f; // TODO:

    UIWidget parent = get_parent_widget();
    UITextWidget textW = parent.node().add_text({}, textWI, widgetS);
    widgetS->widget = (UIWidget)textW;

    return widgetS;
}

UIWidgetState* UIWindowState::get_or_create_text_edit()
{
    UIWidgetState* widgetS = get_or_create_widget_state(UI_WIDGET_TEXT_EDIT);

    if (widgetS->widget && widgetS->widget.get_type() == UI_WIDGET_TEXT_EDIT)
        return widgetS;

    UITextEditWidgetInfo textWI{};
    textWI.fontSize = 16.0f; // TODO:
    textWI.placeHolder = nullptr;
    textWI.onSubmit = &on_text_submit_handler;
    textWI.onChange = &on_text_change_handler;

    UILayoutInfo layoutI{};
    layoutI.sizeX = UISize::fixed(100.0f); // TODO:
    layoutI.sizeY = UISize::fixed(textWI.fontSize * 1.2f);

    UIWidget parent = get_parent_widget();
    UITextEditWidget textW = parent.node().add_text_edit(layoutI, textWI, widgetS);
    widgetS->widget = (UIWidget)textW;

    return widgetS;
}

UIWidgetState* UIWindowState::get_or_create_image(RImage image)
{
    UIWidgetState* widgetS = get_or_create_widget_state(UI_WIDGET_IMAGE);

    if (widgetS->widget && widgetS->widget.get_type() == UI_WIDGET_IMAGE)
        return widgetS;

    UILayoutInfo layoutI{};
    layoutI.sizeX = UISize::fixed(10.0f);
    layoutI.sizeY = UISize::fixed(10.0f);
    UIImageWidgetInfo imageWI{};
    imageWI.image = image;

    UIWidget parent = get_parent_widget();
    UIImageWidget imageW = parent.node().add_image({}, imageWI, widgetS);
    widgetS->widget = (UIWidget)imageW;

    return widgetS;
}

UIWidgetState* UIWindowState::get_or_create_panel()
{
    UIWidgetState* widgetS = get_or_create_widget_state(UI_WIDGET_PANEL);

    if (widgetS->widget && widgetS->widget.get_type() == UI_WIDGET_PANEL)
        return widgetS;

    UILayoutInfo layoutI{};
    layoutI.sizeX = UISize::fit();
    layoutI.sizeY = UISize::fit();
    layoutI.childAxis = UI_AXIS_Y;
    UIPanelWidgetInfo panelWI{};

    UIWidget parent = get_parent_widget();
    UIPanelWidget panelW = parent.node().add_panel(layoutI, panelWI, widgetS);
    widgetS->widget = (UIWidget)panelW;

    return widgetS;
}

UIWidgetState* UIWindowState::get_or_create_toggle()
{
    UIWidgetState* widgetS = get_or_create_widget_state(UI_WIDGET_TOGGLE);

    if (widgetS->widget && widgetS->widget.get_type() == UI_WIDGET_TOGGLE)
        return widgetS;

    UILayoutInfo layoutI{};
    layoutI.sizeX = UISize::grow();
    layoutI.sizeY = UISize::fit();
    UIToggleWidgetInfo toggleWI{};
    toggleWI.state = false;
    toggleWI.on_toggle = [](UIToggleWidget toggle, bool state, void* user) {
        UIWidgetState* widgetS = (UIWidgetState*)user;
        widgetS->isTogglePressed.set(true);
    };

    UIWidget parent = get_parent_widget();
    UIToggleWidget toggleW = parent.node().add_toggle(layoutI, toggleWI, widgetS);
    widgetS->widget = (UIWidget)toggleW;

    return widgetS;
}

UIWidgetState* UIWindowState::get_or_create_scroll(Color bgColor)
{
    UIWidgetState* widgetS = get_or_create_widget_state(UI_WIDGET_SCROLL);

    if (widgetS->widget && widgetS->widget.get_type() == UI_WIDGET_SCROLL)
    {
        UIScrollWidget scrollW = (UIScrollWidget)widgetS->widget;
        scrollW.set_scroll_bg_color(bgColor);
        return widgetS;
    }

    UILayoutInfo layoutI{};
    layoutI.sizeX = UISize::grow();
    layoutI.sizeY = UISize::grow();
    layoutI.childAxis = UI_AXIS_Y;
    UIScrollWidgetInfo scrollWI{};
    scrollWI.bgColor = bgColor;

    UIWidget parent = get_parent_widget();
    UIScrollWidget scrollW = parent.node().add_scroll(layoutI, scrollWI, widgetS);
    widgetS->widget = (UIWidget)scrollW;

    return widgetS;
}

UIWidgetState* UIWindowState::get_or_create_button(const char* text)
{
    UIWidgetState* widgetS = get_or_create_widget_state(UI_WIDGET_BUTTON);

    if (widgetS->widget && widgetS->widget.get_type() == UI_WIDGET_BUTTON)
        return widgetS;

    UILayoutInfo layoutI{};
    layoutI.sizeX = UISize::fixed(100.0f);
    layoutI.sizeY = UISize::fixed(20.0f);
    UIButtonWidgetInfo buttonWI{};
    buttonWI.text = text;
    buttonWI.textColor = 0xFFFFFFFF;
    buttonWI.onClick = [](UIButtonWidget, MouseButton btn, void* user) {
        auto* state = (UIWidgetState*)user;
        state->isButtonPressed.set(true);
    };

    UIWidget parent = get_parent_widget();
    UIButtonWidget buttonW = parent.node().add_button(layoutI, buttonWI, widgetS);
    widgetS->widget = (UIWidget)buttonW;

    return widgetS;
}

UIWidgetState* UIWindowState::get_or_create_slider()
{
    UIWidgetState* widgetS = get_or_create_widget_state(UI_WIDGET_SLIDER);

    if (widgetS->widget && widgetS->widget.get_type() == UI_WIDGET_SLIDER)
        return widgetS;

    UILayoutInfo layoutI{};
    layoutI.sizeX = UISize::grow();
    layoutI.sizeY = UISize::fixed(20.0f);
    UISliderWidgetInfo sliderWI{};
    sliderWI.min = 0.0f;
    sliderWI.max = 1.0f;

    UIWidget parent = get_parent_widget();
    UISliderWidget sliderW = parent.node().add_slider(layoutI, sliderWI, widgetS);
    widgetS->widget = (UIWidget)sliderW;

    return widgetS;
}

//
// Public Immediate Mode API
//

void ui_imgui_release(UIContext ctx)
{
    if (!sImFrames.contains(ctx.unwrap()))
        return;

    LD_ASSERT(!sImFrame && "ui_frame_end not called");
    sImFrame = sImFrames[ctx.unwrap()];

    while (!sImFrame->imWindows.empty())
    {
        UIWindowState* windowS = sImFrame->imWindows.begin()->second;
        destroy_window_state(windowS);
    }

    heap_delete<UIImmediateFrame>(sImFrame);
    sImFrame = nullptr;

    sImFrames.erase(ctx.unwrap());
}

void ui_frame_begin(UIContext ctx, const Vec2& screenExtent)
{
    LD_ASSERT(!sImFrame && "ui_frame_end not called");

    sImFrame = get_or_create_immediate_frame(ctx, screenExtent);
    sImFrame->popupLayer.raise();

    ctx.set_user(sImFrame);
    ctx.set_on_event(&on_ui_context_event);
}

void ui_frame_end()
{
    LD_ASSERT(sImFrame && "ui_frame_begin not called");
    LD_ASSERT(!sImFrame->imWindow && "ui_pop_window not called");

    sImFrame = nullptr;
}

void ui_top_layout(const UILayoutInfo& layoutI)
{
    LD_ASSERT_UI_TOP_WIDGET;

    UIWidgetState* widgetS = sImFrame->imWindow->imWidgetStack.top();
    widgetS->widget.set_layout(layoutI);
}

void ui_top_layout_size(const UISize& sizeX, const UISize& sizeY)
{
    LD_ASSERT_UI_TOP_WIDGET;

    UIWidgetState* widgetS = sImFrame->imWindow->imWidgetStack.top();
    widgetS->widget.set_layout_size(sizeX, sizeY);
}

void ui_top_layout_child_axis(UIAxis childAxis)
{
    LD_ASSERT_UI_TOP_WIDGET;

    UIWidgetState* widgetS = sImFrame->imWindow->imWidgetStack.top();
    widgetS->widget.set_layout_child_axis(childAxis);
}

void ui_top_layout_child_padding(const UIPadding& pad)
{
    LD_ASSERT_UI_TOP_WIDGET;

    UIWidgetState* widgetS = sImFrame->imWindow->imWidgetStack.top();
    widgetS->widget.set_layout_child_padding(pad);
}

void ui_top_layout_child_gap(float gap)
{
    LD_ASSERT_UI_TOP_WIDGET;

    UIWidgetState* widgetS = sImFrame->imWindow->imWidgetStack.top();
    widgetS->widget.set_layout_child_gap(gap);
}

void ui_top_user(void* imUser)
{
    LD_ASSERT_UI_TOP_WIDGET;

    UIWidgetState* widgetS = sImFrame->imWindow->imWidgetStack.top();
    widgetS->imUser = imUser;
}

void ui_top_rect(Rect& outRect)
{
    LD_ASSERT_UI_TOP_WIDGET;

    UIWidgetState* widgetS = sImFrame->imWindow->imWidgetStack.top();
    outRect = widgetS->widget.get_rect();
}

void ui_top_draw(const IMDrawCallback& imDrawCallback)
{
    LD_ASSERT_UI_TOP_WIDGET;

    UIWidgetState* widgetS = sImFrame->imWindow->imWidgetStack.top();
    widgetS->onDraw = imDrawCallback;
    widgetS->widget.set_on_draw([](UIWidget widget, ScreenRenderComponent renderer) {
        UIWidgetState* widgetS = (UIWidgetState*)widget.get_user();
        widgetS->onDraw(widgetS->widget, renderer, widgetS->imUser);
    });
}

bool ui_top_drag(MouseButton& dragBtn, Vec2& dragPos, bool& dragBegin)
{
    LD_ASSERT_UI_TOP_WIDGET;

    UIWidgetState* widgetS = sImFrame->imWindow->imWidgetStack.top();

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

    UIWidgetState* widgetS = sImFrame->imWindow->imWidgetStack.top();

    return widgetS->widget.is_dragged();
}

bool ui_top_hover(UIEventType& outHover)
{
    LD_ASSERT_UI_TOP_WIDGET;

    UIWidgetState* widgetS = sImFrame->imWindow->imWidgetStack.top();

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

    UIWidgetState* widgetS = sImFrame->imWindow->imWidgetStack.top();

    return widgetS->widget.is_hovered();
}

bool ui_top_scroll(Vec2& scroll)
{
    LD_ASSERT_UI_TOP_WIDGET;

    UIWidgetState* widgetS = sImFrame->imWindow->imWidgetStack.top();

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

bool ui_top_mouse_down(MouseButton& outButton)
{
    LD_ASSERT_UI_TOP_WIDGET;

    UIWidgetState* widgetS = sImFrame->imWindow->imWidgetStack.top();

    if (!widgetS->widget.has_on_event())
        widgetS->widget.set_consume_mouse_event(true);

    UIEvent event;
    if (widgetS->consume_event(UI_EVENT_MOUSE_DOWN, event))
    {
        outButton = event.mouse.button;
        return true;
    }

    return false;
}

bool ui_top_mouse_up(MouseButton& outButton)
{
    LD_ASSERT_UI_TOP_WIDGET;

    UIWidgetState* widgetS = sImFrame->imWindow->imWidgetStack.top();

    if (!widgetS->widget.has_on_event())
        widgetS->widget.set_consume_mouse_event(true);

    UIEvent event;
    if (widgetS->consume_event(UI_EVENT_MOUSE_UP, event))
    {
        outButton = event.mouse.button;
        return true;
    }

    return false;
}

bool ui_top_key_down(KeyValue& outKey)
{
    LD_ASSERT_UI_TOP_WIDGET;

    UIWidgetState* widgetS = sImFrame->imWindow->imWidgetStack.top();

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

    UIWidgetState* widgetS = sImFrame->imWindow->imWidgetStack.top();

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
    LD_ASSERT(sImFrame->imWindow && "missing window");
    LD_ASSERT(!sImFrame->imWindow->imWidgetStack.empty() && "widget stack empty");

    UIWindowState* windowS = sImFrame->imWindow;
    UIWidgetState* parentS = windowS->imWidgetStack.top();

    // make sure events don't bleed into next frame
    parentS->events.clear();

    for (int i = parentS->childCounter; i < (int)parentS->children.size(); i++)
    {
        // Widget tree hierarchy has changed compared to previous frame.
        // Trim excessive subtrees.
        destroy_widget_subtree(windowS->widgetStatePA, parentS->children[i]);
    }
    parentS->children.resize(parentS->childCounter);

    sImFrame->imWindow->imWidgetStack.pop();
}

void ui_pop_window()
{
    LD_ASSERT(sImFrame->imWindow && "missing window");
    LD_ASSERT(sImFrame->imWindow->imWidgetStack.size() == 1 && "some widget pushed but not popped");

    // last widget state on stack is the window
    ui_pop();

    sImFrame->imWindow = nullptr;
}

void ui_push_window(UIWindow client)
{
    LD_PROFILE_SCOPE;
    LD_ASSERT_UI_PUSH_WINDOW;

    Hash64 windowHash = client.get_hash();

    // If client is different from last time, invalidate window state.
    if (sImFrame->imWindows.contains(windowHash))
    {
        UIWindowState* windowS = sImFrame->imWindows[windowHash];
        if (windowS->window.unwrap() != client.unwrap())
        {
            destroy_window_state(windowS);
            sImFrame->imWindows.erase(windowHash);
        }
    }

    sImFrame->set_window_state(get_or_create_window_state(client));
}

void ui_window_set_color(Color color)
{
    LD_ASSERT_UI_WINDOW;

    sImFrame->imWindow->window.set_color(color);
}

void ui_window_set_rect(const Rect& rect)
{
    LD_ASSERT_UI_WINDOW;

    sImFrame->imWindow->window.set_rect(rect);
}

bool ui_window_has_client(const char* name)
{
    LD_ASSERT_UI_FRAME_BEGIN;

    return sImFrame->imWindows.contains(Hash64(name));
}

void ui_request_popup_window(const char* popupName, const Vec2& position)
{
    ui_clear_popup_window();

    UIWindow popupW = sImFrame->get_or_create_popup_window(popupName);

    sImFrame->imPopup = get_or_create_window_state(popupW);
    sImFrame->imPopup->window.hide();
    sImFrame->imPopup->window.set_pos(position);
}

void ui_clear_popup_window()
{
    LD_ASSERT_UI_FRAME_BEGIN;

    sImFrame->clear_popup_window();
}

bool ui_push_popup_window(const char* popupName)
{
    LD_ASSERT_UI_FRAME_BEGIN;
    LD_ASSERT_UI_PUSH_WINDOW;

    Hash64 hash = hash64_FNV_1a(popupName, strlen(popupName));

    if (!sImFrame->imPopup)
        return false;

    std::string currentPopupName;
    UIWindow popupW = sImFrame->imPopup->window;
    popupW.get_name(currentPopupName);
    if (currentPopupName != popupName)
        return false;

    popupW.show();

    sImFrame->set_window_state(sImFrame->imPopup);

    return true;
}

void ui_push_text(const char* text)
{
    LD_ASSERT_UI_PUSH;

    UIWindowState* imWindow = sImFrame->imWindow;
    UIWidgetState* imWidget = imWindow->get_or_create_text();
    UITextWidget textW = (UITextWidget)imWidget->widget;
    LD_ASSERT(textW.get_type() == UI_WIDGET_TEXT);

    textW.set_text(text);

    imWindow->imWidgetStack.push(imWidget);
}

void ui_text_style(Color color, FontAtlas fontAtlas, RImage fontImage)
{
    LD_ASSERT_UI_TOP_WIDGET_TYPE(UI_WIDGET_TEXT);

    UIWidgetState* imWidget = sImFrame->imWindow->imWidgetStack.top();
    UITextWidget textW = (UITextWidget)imWidget->widget;

    textW.set_text_style(color, fontAtlas, fontImage);
}

void ui_push_text_edit(UITextEditDomain domain)
{
    LD_ASSERT_UI_PUSH;

    UIWindowState* imWindow = sImFrame->imWindow;
    UIWidgetState* imWidget = imWindow->get_or_create_text_edit();
    UITextEditWidget textW = (UITextEditWidget)imWidget->widget;
    LD_ASSERT(textW.get_type() == UI_WIDGET_TEXT_EDIT);

    textW.set_domain(domain);

    imWindow->imWidgetStack.push(imWidget);
}

void ui_text_edit_set_text(View text)
{
    LD_ASSERT_UI_TOP_WIDGET_TYPE(UI_WIDGET_TEXT_EDIT);

    UIWidgetState* imWidget = sImFrame->imWindow->imWidgetStack.top();
    UITextEditWidget editW = (UITextEditWidget)imWidget->widget;

    editW.set_text(text);
}

bool ui_text_edit_changed(std::string& text)
{
    LD_ASSERT_UI_TOP_WIDGET_TYPE(UI_WIDGET_TEXT_EDIT);

    UIWidgetState* imWidget = sImFrame->imWindow->imWidgetStack.top();

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

    UIWidgetState* imWidget = sImFrame->imWindow->imWidgetStack.top();

    if (imWidget->textEdit.isSubmitted.read())
    {
        text = imWidget->textEdit.lastSubmission;
        imWidget->textEdit.lastSubmission.clear();
        return true;
    }

    return false;
}

void ui_push_image(RImage image, float width, float height, Color tint, const Rect* portion)
{
    LD_ASSERT_UI_PUSH;

    UIWindowState* imWindow = sImFrame->imWindow;
    UIWidgetState* imWidget = imWindow->get_or_create_image(image);
    UIImageWidget imageW = (UIImageWidget)imWidget->widget;
    LD_ASSERT(imageW.get_type() == UI_WIDGET_IMAGE);

    imageW.set_layout_size(UISize::fixed(width), UISize::fixed(height));
    imageW.set_image_tint(tint);

    if (portion)
        imageW.set_image_rect(*portion);

    imWindow->imWidgetStack.push(imWidget);
}

void ui_push_panel()
{
    LD_ASSERT_UI_PUSH;

    UIWindowState* imWindow = sImFrame->imWindow;
    UIWidgetState* imWidget = imWindow->get_or_create_panel();

    imWindow->imWidgetStack.push(imWidget);
}

void ui_panel_color(Color color)
{
    LD_ASSERT_UI_TOP_WIDGET_TYPE(UI_WIDGET_PANEL);

    UIWidgetState* imWidget = sImFrame->imWindow->imWidgetStack.top();
    UIPanelWidget panel = (UIPanelWidget)imWidget->widget;

    *panel.panel_color() = color;
}

void ui_push_toggle(bool& isPressed, bool& state)
{
    LD_ASSERT_UI_PUSH;

    UIWindowState* imWindow = sImFrame->imWindow;
    UIWidgetState* imWidget = imWindow->get_or_create_toggle();
    UIToggleWidget toggleW = (UIToggleWidget)imWidget->widget;
    LD_ASSERT(toggleW.get_type() == UI_WIDGET_TOGGLE);

    isPressed = imWidget->isTogglePressed.read();
    state = toggleW.get_state();

    imWindow->imWidgetStack.push(imWidget);
}

void ui_push_scroll(Color bgColor)
{
    LD_ASSERT_UI_PUSH;

    UIWindowState* imWindow = sImFrame->imWindow;
    UIWidgetState* imWidget = imWindow->get_or_create_scroll(bgColor);

    imWindow->imWidgetStack.push(imWidget);
}

void ui_push_button(const char* text, bool& isPressed)
{
    LD_ASSERT_UI_PUSH;

    UIWindowState* imWindow = sImFrame->imWindow;
    UIWidgetState* imWidget = imWindow->get_or_create_button(text);

    isPressed = imWidget->isButtonPressed.read();

    imWindow->imWidgetStack.push(imWidget);
}

void ui_push_slider(float minValue, float maxValue, float* value)
{
    LD_ASSERT_UI_PUSH;

    UIWindowState* imWindow = sImFrame->imWindow;
    UIWidgetState* imWidget = imWindow->get_or_create_slider();
    UISliderWidget sliderW = (UISliderWidget)imWidget->widget;
    LD_ASSERT(sliderW.get_type() == UI_WIDGET_SLIDER);

    sliderW.set_value_range(minValue, maxValue);

    if (value)
        *value = sliderW.get_value();

    imWindow->imWidgetStack.push(imWidget);
}

} // namespace LD