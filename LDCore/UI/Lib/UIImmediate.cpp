#include <Ludens/Header/Assert.h>
#include <Ludens/Header/Impulse.h>
#include <Ludens/Header/Types.h>
#include <Ludens/System/Allocator.h>
#include <Ludens/System/Memory.h>
#include <Ludens/UI/UIImmediate.h>
#include <stack>
#include <unordered_map>
#include <vector>

#define LD_ASSERT_UI_PUSH_WINDOW                            \
    LD_ASSERT(sImFrame.ctx && "ui_frame_begin not called"); \
    LD_ASSERT(!sImFrame.imWindow && "ui_window_begin already called");

#define LD_ASSERT_UI_PUSH LD_ASSERT(sImFrame.imWindow && "ui_window_begin not called")

namespace LD {

struct UIWidgetState
{
    UIWidget widget = {};                 // actual retained widget
    std::vector<UIWidgetState*> children; // direct children, retained across frames
    Hash32 widgetHash;                    // hash that identifies this state uniquely in its window
    Impulse isPressed;
    Impulse isReleased;
    int childCounter = 0; // number of children widget states in this frame
    union
    {
        Impulse isTogglePressed;
        Impulse isButtonPressed;
    };
};

struct UIWindowState
{
    UIWindow window;
    PoolAllocator widgetStatePA;
    UIWidgetState* state; // widget state for the window itself
    std::stack<UIWidgetState*> imWidgetStack;
    Hash32 windowHash;
    bool hasCreatedWindow;

    UIWindowState();
    ~UIWindowState();

    inline UIWidget get_parent_widget()
    {
        return imWidgetStack.empty() ? (UIWidget)window : (UIWidget)imWidgetStack.top()->widget;
    }

    UIWidgetState* get_or_create_text();
    UIWidgetState* get_or_create_image(RImage image);
    UIWidgetState* get_or_create_panel();
    UIWidgetState* get_or_create_toggle();
    UIWidgetState* get_or_create_scroll();
    UIWidgetState* get_or_create_button(const char* text);
    UIWidgetState* get_or_create_slider();
};

struct UIImmediateFrame
{
    UIContext ctx;           // connected external context
    UIWindowState* imWindow; // current window
};

static UIImmediateFrame sImFrame;
static std::unordered_map<Hash32, UIWindowState*> sImWindows;

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
static Hash32 get_widget_state_hash(UIWidgetType type, int siblingIndex, Hash32 parentStateHash)
{
    size_t hash64 = (size_t)parentStateHash;

    hash_combine(hash64, type);
    hash_combine(hash64, siblingIndex);

    return (Hash32)hash64;
}

// NOTE: has side effect of incrementing the childCounter of top widget
static UIWidgetState* get_or_create_widget_state(std::stack<UIWidgetState*>& stack, PoolAllocator statePA, UIWidgetType type)
{
    LD_ASSERT(!stack.empty());

    UIWidgetState* parentS = stack.top();
    Hash32 parentHash = parentS->widgetHash;
    int siblingIndex = parentS->childCounter++;

    if (siblingIndex >= (int)parentS->children.size())
    {
        int oldSize = (int)parentS->children.size();
        parentS->children.resize(siblingIndex + 1);
        for (int i = oldSize; i < siblingIndex + 1; i++)
            parentS->children[i] = nullptr;
    }

    Hash32 widgetHash = get_widget_state_hash(type, siblingIndex, parentHash);
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
                destroy_widget_subtree(statePA, parentS->children[i]);

            LD_ASSERT(parentS->childCounter == siblingIndex + 1);
            parentS->children.resize(parentS->childCounter);
            widgetS = parentS->children[siblingIndex] = nullptr;
        }
    }

    if (!widgetS)
    {
        widgetS = parentS->children[siblingIndex] = (UIWidgetState*)statePA.allocate();
        new (widgetS) UIWidgetState();
        widgetS->widget = {}; // caller creates
        widgetS->widgetHash = widgetHash;
        widgetS->childCounter = 0;
    }

    return widgetS;
}

static UIWindowState* get_or_create_window_state(Hash32 windowName, bool createWindowHandle)
{
    if (sImWindows.contains(windowName))
        return sImWindows[windowName];

    UIWindowState* windowS = heap_new<UIWindowState>(MEMORY_USAGE_UI);
    windowS->windowHash = windowName;
    windowS->window = {};
    windowS->hasCreatedWindow = !createWindowHandle;
    windowS->state = (UIWidgetState*)windowS->widgetStatePA.allocate();
    new (windowS->state) UIWidgetState(); // TODO: placement delete
    UIWidgetState* widgetS = windowS->state;
    widgetS->widget = {};
    widgetS->widgetHash = windowS->windowHash;

    if (createWindowHandle)
    {
        UILayoutInfo layoutI{};
        layoutI.sizeX = UISize::fit();
        layoutI.sizeY = UISize::fit();
        layoutI.childAxis = UI_AXIS_Y;
        layoutI.childGap = 5.0f;
        layoutI.childPadding = {.left = 5.0f, .right = 5.0f, .top = 5.0f, .bottom = 5.0f};
        UIWindowInfo windowI{};
        windowI.defaultMouseControls = false;
        windowI.drawWithScissor = false;
        windowI.hidden = true;
        windowI.name = nullptr;
        windowS->window = sImFrame.ctx.add_window(layoutI, windowI, windowS);
        windowS->window.raise();
        windowS->window.set_on_draw([](UIWidget widget, ScreenRenderComponent renderer) {
            renderer.draw_rect(widget.get_rect(), Color(0x303030FF));
        });

        widgetS->widget = (UIWidget)windowS->window;
    }

    sImWindows[windowName] = windowS;

    return windowS;
}

// NOTE: modifies sImWindows, caller should not iterate upon
static void destroy_window_state(UIWindowState* state)
{
    LD_ASSERT(sImFrame.ctx && state->window);
    LD_ASSERT(sImFrame.ctx.unwrap() == state->window.node().get_context());

    if (!sImWindows.contains(state->windowHash))
        return;

    sImWindows.erase(state->windowHash);
    heap_delete<UIWindowState>(state);
}

UIWindowState::UIWindowState()
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
    UIWidgetState* widgetS = get_or_create_widget_state(imWidgetStack, widgetStatePA, UI_WIDGET_TEXT);

    if (widgetS->widget && widgetS->widget.get_type() == UI_WIDGET_TEXT)
        return widgetS;

    UITextWidgetInfo textWI{};
    textWI.cstr = nullptr;
    textWI.fontSize = 16.0f; // TODO:
    textWI.hoverHL = false;

    UIWidget parent = get_parent_widget();
    UITextWidget textW = parent.node().add_text({}, textWI, widgetS);
    widgetS->widget = (UIWidget)textW;

    return widgetS;
}

UIWidgetState* UIWindowState::get_or_create_image(RImage image)
{
    UIWidgetState* widgetS = get_or_create_widget_state(imWidgetStack, widgetStatePA, UI_WIDGET_IMAGE);

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
    UIWidgetState* widgetS = get_or_create_widget_state(imWidgetStack, widgetStatePA, UI_WIDGET_PANEL);

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
    UIWidgetState* widgetS = get_or_create_widget_state(imWidgetStack, widgetStatePA, UI_WIDGET_TOGGLE);

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

UIWidgetState* UIWindowState::get_or_create_scroll()
{
    UIWidgetState* widgetS = get_or_create_widget_state(imWidgetStack, widgetStatePA, UI_WIDGET_SCROLL);

    if (widgetS->widget && widgetS->widget.get_type() == UI_WIDGET_SCROLL)
        return widgetS;

    UILayoutInfo layoutI{};
    layoutI.sizeX = UISize::grow();
    layoutI.sizeY = UISize::grow();
    layoutI.childAxis = UI_AXIS_Y;
    UIScrollWidgetInfo scrollWI{};
    scrollWI.hasScrollBar = false;

    UIWidget parent = get_parent_widget();
    UIScrollWidget scrollW = parent.node().add_scroll(layoutI, scrollWI, widgetS);
    widgetS->widget = (UIWidget)scrollW;

    return widgetS;
}

UIWidgetState* UIWindowState::get_or_create_button(const char* text)
{
    UIWidgetState* widgetS = get_or_create_widget_state(imWidgetStack, widgetStatePA, UI_WIDGET_BUTTON);

    if (widgetS->widget && widgetS->widget.get_type() == UI_WIDGET_BUTTON)
        return widgetS;

    UILayoutInfo layoutI{};
    layoutI.sizeX = UISize::fixed(100.0f);
    layoutI.sizeY = UISize::fixed(20.0f);
    UIButtonWidgetInfo buttonWI{};
    buttonWI.text = text;
    buttonWI.textColor = 0xFFFFFFFF;
    buttonWI.on_press = [](UIButtonWidget w, MouseButton btn, void* user) {
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
    UIWidgetState* widgetS = get_or_create_widget_state(imWidgetStack, widgetStatePA, UI_WIDGET_SLIDER);

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
    LD_ASSERT(!sImFrame.ctx && "ui_frame_end not called");

    sImFrame.ctx = ctx;

    while (!sImWindows.empty())
    {
        UIWindowState* windowS = sImWindows.begin()->second;
        destroy_window_state(windowS);
    }

    sImFrame.ctx = {};
}

void ui_frame_begin(UIContext ctx)
{
    LD_ASSERT(!sImFrame.ctx && "ui_frame_end not called");

    sImFrame.ctx = ctx;
}

void ui_frame_end()
{
    LD_ASSERT(sImFrame.ctx && "ui_frame_begin not called");

    sImFrame.ctx = {};
    sImFrame.imWindow = nullptr;
}

void ui_pop()
{
    LD_ASSERT(sImFrame.imWindow && "missing window");
    LD_ASSERT(!sImFrame.imWindow->imWidgetStack.empty() && "widget stack empty");

    UIWindowState* windowS = sImFrame.imWindow;
    UIWidgetState* parentS = windowS->imWidgetStack.top();

    for (int i = parentS->childCounter; i < (int)parentS->children.size(); i++)
    {
        // Widget tree hierarchy has changed compared to previous frame.
        // Trim excessive subtrees.
        destroy_widget_subtree(windowS->widgetStatePA, parentS->children[i]);
    }
    parentS->children.resize(parentS->childCounter);

    sImFrame.imWindow->imWidgetStack.pop();
}

void ui_pop_window()
{
    LD_ASSERT(sImFrame.imWindow && "missing window");
    LD_ASSERT(sImFrame.imWindow->imWidgetStack.size() == 1 && "some widget pushed but not popped");

    // last widget state on stack is the window
    ui_pop();

    sImFrame.imWindow = nullptr;
}

void ui_push_window(const char* name)
{
    LD_ASSERT_UI_PUSH_WINDOW;

    Hash32 windowName(name);
    UIWindowState* windowS = sImFrame.imWindow = get_or_create_window_state(windowName, true);

    while (!windowS->imWidgetStack.empty())
        windowS->imWidgetStack.pop();

    windowS->state->childCounter = 0; // track widget hierarchy each frame
    windowS->state->widget = windowS->window;
    windowS->imWidgetStack.push(windowS->state);
    windowS->window.show();
}

void ui_push_window_client(const char* name, UIWindow client)
{
    LD_ASSERT_UI_PUSH_WINDOW;

    Hash32 windowName(name);
    UIWindowState* imWindow = sImFrame.imWindow = get_or_create_window_state(windowName, false);
    imWindow->state->childCounter = 0;
    imWindow->state->widget = (UIWidget)client;
    imWindow->imWidgetStack.push(imWindow->state);
    imWindow->window = client;
    imWindow->window.show();
}

void ui_push_text(const char* text)
{
    LD_ASSERT_UI_PUSH;

    UIWindowState* imWindow = sImFrame.imWindow;
    UIWidgetState* imWidget = imWindow->get_or_create_text();
    UITextWidget textW = (UITextWidget)imWidget->widget;
    LD_ASSERT(textW.get_type() == UI_WIDGET_TEXT);

    textW.set_text(text);

    imWindow->imWidgetStack.push(imWidget);
}

void ui_push_image(RImage image, float width, float height)
{
    LD_ASSERT_UI_PUSH;

    UIWindowState* imWindow = sImFrame.imWindow;
    UIWidgetState* imWidget = imWindow->get_or_create_image(image);
    UITextWidget imageW = (UITextWidget)imWidget->widget;
    LD_ASSERT(imageW.get_type() == UI_WIDGET_IMAGE);

    imageW.set_layout_size(UISize::fixed(width), UISize::fixed(height));

    imWindow->imWidgetStack.push(imWidget);
}

void ui_push_panel()
{
    LD_ASSERT_UI_PUSH;

    UIWindowState* imWindow = sImFrame.imWindow;
    UIWidgetState* imWidget = imWindow->get_or_create_panel();

    imWindow->imWidgetStack.push(imWidget);
}

void ui_push_toggle(bool& isPressed, bool& state)
{
    LD_ASSERT_UI_PUSH;

    UIWindowState* imWindow = sImFrame.imWindow;
    UIWidgetState* imWidget = imWindow->get_or_create_toggle();
    UIToggleWidget toggleW = (UIToggleWidget)imWidget->widget;
    LD_ASSERT(toggleW.get_type() == UI_WIDGET_TOGGLE);

    isPressed = imWidget->isTogglePressed.read();
    state = toggleW.get_state();

    imWindow->imWidgetStack.push(imWidget);
}

void ui_push_scroll()
{
    LD_ASSERT_UI_PUSH;

    UIWindowState* imWindow = sImFrame.imWindow;
    UIWidgetState* imWidget = imWindow->get_or_create_scroll();

    imWindow->imWidgetStack.push(imWidget);
}

void ui_push_button(const char* text, bool& isPressed)
{
    LD_ASSERT_UI_PUSH;

    UIWindowState* imWindow = sImFrame.imWindow;
    UIWidgetState* imWidget = imWindow->get_or_create_button(text);

    isPressed = imWidget->isButtonPressed.read();

    imWindow->imWidgetStack.push(imWidget);
}

void ui_push_slider(float minValue, float maxValue, float* value)
{
    LD_ASSERT_UI_PUSH;

    UIWindowState* imWindow = sImFrame.imWindow;
    UIWidgetState* imWidget = imWindow->get_or_create_slider();
    UISliderWidget sliderW = (UISliderWidget)imWidget->widget;
    LD_ASSERT(sliderW.get_type() == UI_WIDGET_SLIDER);

    sliderW.set_value_range(minValue, maxValue);

    if (value)
        *value = sliderW.get_value();

    imWindow->imWidgetStack.push(imWidget);
}

} // namespace LD