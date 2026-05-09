#pragma once

#include <Ludens/Text/TextBuffer.h>
#include <Ludens/Text/TextEditLite.h>
#include <Ludens/UI/UIDef.h>
#include <Ludens/UI/UIFont.h>
#include <Ludens/UI/UIWidget.h>

namespace LD {

typedef void (*UITextEditOnChange)(UIWidget widget, View text, void* user);
typedef void (*UITextEditOnSubmit)(UIWidget widget, View text, void* user);

class UITextEditData
{
    friend struct UITextEditWidgetObj;

public:
    UITextEditOnChange onChange = nullptr;
    UITextEditOnSubmit onSubmit = nullptr;
    UIFont font = {};                     /// displayed font
    float fontSize = UIFont::base_size(); /// rendered size
    Color bgColor = 0;                    /// if non zero, the background color when not editing
    Color bgColorEdit = 0;                /// if non zero, the background color during edit
    bool beginEditOnFocus = true;         /// whether the widget should start editing upon focus (mouse click)

    UITextEditData();
    UITextEditData(const UITextEditData& other);
    ~UITextEditData();

    UITextEditData& operator=(const UITextEditData& other);

    inline String get_text() { return mEditor.get_string(); }

    /// @brief Set text field value, does not trigger on_change callbacks.
    void set_text(View text);

    /// @brief Domain influences how key input is treated and may change the final text before submission.
    /// @note Upon domain change, text value is reset.
    void set_domain(UITextEditDomain domain);

    inline bool is_editing() const { return mIsEditing; }

private:
    UITextEditDomain mDomain = UI_TEXT_EDIT_DOMAIN_STRING;
    TextEditLite mEditor = {};
    String mOriginal = {}; /// text value before edit
    size_t mDragBeginPos = 0;
    size_t mDragPos = 0;
    bool mIsEditing = false;
};

struct UITextEditWidget : UIWidget
{
    bool try_begin_edit();
    bool is_editing();

    void set_on_change(UITextEditOnChange onChange);
    void set_on_submit(UITextEditOnSubmit onSubmit);
};

} // namespace LD