#pragma once

#include <Ludens/Text/TextBuffer.h>
#include <Ludens/Text/TextEditLite.h>
#include <Ludens/UI/UIFont.h>
#include <Ludens/UI/UIWidget.h>

namespace LD {

enum UITextEditDomain
{
    UI_TEXT_EDIT_DOMAIN_STRING = 0,
    UI_TEXT_EDIT_DOMAIN_UINT,
    UI_TEXT_EDIT_DOMAIN_F32,
};

typedef void (*UITextEditOnChange)(UIWidget widget, View text, void* user);
typedef void (*UITextEditOnSubmit)(UIWidget widget, View text, void* user);

struct UITextEditStorage
{
    TextEditLite editor = {};             /// text buffer editor handle
    std::string original = {};            /// text value before edit
    UIFont font = {};                     /// displayed font
    UITextEditDomain domain = {};         /// input domain, filters key input
    float fontSize = UIFont::base_size(); /// rendered size

    UITextEditStorage();
    UITextEditStorage(const UITextEditStorage& other);
    ~UITextEditStorage();

    UITextEditStorage& operator=(const UITextEditStorage& other);

    inline std::string get_text() { return editor.get_string(); }

    /// @brief Set text field value, does not trigger on_change callbacks.
    void set_text(const std::string& text);

    /// @brief Domain influences how key input is treated and may change the final text before submission.
    /// @note Upon domain change, text value is reset.
    void set_domain(UITextEditDomain domain);
};

struct UITextEditWidget : UIWidget
{
    UITextEditStorage* get_storage();

    bool is_editing();

    void set_on_change(UITextEditOnChange onChange);
    void set_on_submit(UITextEditOnSubmit onSubmit);
};

} // namespace LD