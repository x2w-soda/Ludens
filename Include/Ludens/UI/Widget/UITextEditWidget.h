#pragma once

#include <Ludens/Text/TextBuffer.h>
#include <Ludens/UI/UIWidget.h>

namespace LD {

enum UITextEditDomain
{
    UI_TEXT_EDIT_DOMAIN_STRING = 0,
    UI_TEXT_EDIT_DOMAIN_UINT,
};

typedef void (*UITextEditOnChange)(UIWidget widget, View text, void* user);
typedef void (*UITextEditOnSubmit)(UIWidget widget, View text, void* user);

struct UITextEditStorage
{
    TextBuffer<char> buf = {};                            /// current text content
    UITextEditDomain domain = UI_TEXT_EDIT_DOMAIN_STRING; /// input domain, filters key input
    float fontSize = 16.0f;                               /// rendered size

    UITextEditStorage();
    UITextEditStorage(const UITextEditStorage& other);
    ~UITextEditStorage();

    UITextEditStorage& operator=(const UITextEditStorage& other);
};

struct UITextEditWidget : UIWidget
{
    /// @brief Set text edit field value, does not trigger on_change callbacks.
    void set_text(View text);

    /// @brief Domain influences how key input is treated and may change the final text before submission.
    /// @note Upon domain change, text value is reset.
    void set_domain(UITextEditDomain domain);

    void set_on_change(UITextEditOnChange onChange);
    void set_on_submit(UITextEditOnSubmit onSubmit);

    /// @brief Default text edit widget rendering.
    static void on_draw(UIWidget widget, ScreenRenderComponent renderer);
};

} // namespace LD