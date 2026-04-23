#include <Ludens/Asset/AssetRegistry.h>
#include <Ludens/Header/MouseValue.h>
#include <LudensEditor/EditorWidget/EUIRow.h>

#include "EUI.h"

namespace LD {

bool eui_row_label_text_edit(const char* label, UITextEditData* edit, std::string& outText)
{
    EditorTheme theme = eui_get_theme();
    UILayoutInfo layoutI = theme.make_text_row_layout();

    ui_push_text(nullptr, label);
    ui_top_layout(layoutI);
    ui_pop();

    ui_push_text_edit(edit);
    bool hasChanged = ui_text_edit_changed(outText);
    ui_top_layout(layoutI);
    ui_pop();

    return hasChanged;
}

void eui_push_row_scroll(UIScrollData* scroll)
{
    EditorTheme theme = eui_get_theme();

    UILayoutInfo layoutI{};
    layoutI.childAxis = UI_AXIS_Y;
    layoutI.childGap = theme.get_child_gap();
    layoutI.sizeX = UISize::grow();
    layoutI.sizeY = UISize::grow();
    scroll->bgColor = theme.get_ui_theme().get_surface_color();
    ui_push_scroll(scroll);
    ui_top_layout(layoutI);
}

void eui_pop_row_scroll()
{
    ui_pop();
}

bool EUILabelRow::update(const char* label, int rowIndex, bool isHighlighted, std::string& outNewLabel)
{
    EditorTheme theme = eui_get_theme();
    const float textRowHeight = theme.get_text_row_height();

    outNewLabel.clear();

    UILayoutInfo layoutI{};
    layoutI.childAxis = UI_AXIS_X;
    layoutI.childGap = theme.get_child_gap();
    layoutI.childPadding.left = 10.0f;
    layoutI.sizeX = UISize::grow();
    layoutI.sizeY = UISize::fixed(textRowHeight);

    auto* panelData = (UIPanelData*)ui_push_panel(nullptr).get_data();
    ui_top_layout(layoutI);

    Color panelColor = theme.get_ui_theme().get_surface_color();
    if (isHighlighted)
        panelColor = theme.get_ui_theme().get_selection_color();

    if (rowIndex % 2)
        panelColor = Color::lift(panelColor, 0.02f);

    if (ui_top_is_hovered())
        panelColor = Color::lift(panelColor, 0.06f);

    panelData->color = panelColor;

    KeyValue keyVal;
    Vec2 mousePos;
    MouseValue mouseVal;
    bool isRequestingEdit = false;
    bool isSelected = false;
    if (ui_top_mouse_down(mouseVal, mousePos))
        isSelected = true;
    if (ui_top_key_down(keyVal) && keyVal.code() == KEY_CODE_F2)
        isRequestingEdit = true;

    mLabel.bgColor = 0;
    mLabel.beginEditOnFocus = false;
    UITextEditWidget editW = ui_push_text_edit(&mLabel);

    layoutI.sizeX = UISize::grow();
    ui_top_layout(layoutI);

    if (ui_text_edit_submitted(outNewLabel))
        ;
    if (ui_top_mouse_down(mouseVal, mousePos))
        isSelected = true;
    if (!editW.is_editing())
        mLabel.set_text(label);
    if (isRequestingEdit)
        (void)editW.try_begin_edit();
    ui_pop();

    ui_pop();

    return isSelected;
}

template <size_t TCount>
int EUIButtonRow<TCount>::update()
{
    int hasPressed = 0;

    EditorTheme theme = eui_get_theme();
    const float textRowHeight = theme.get_text_row_height();
    UILayoutInfo layoutI{};
    layoutI.sizeX = UISize::grow();
    layoutI.sizeY = UISize::fixed(textRowHeight);
    layoutI.childAlignX = UI_ALIGN_END;
    layoutI.childGap = theme.get_child_gap_large();
    layoutI.childPadding = UIPadding(theme.get_child_pad());

    ui_push_panel(nullptr);
    ui_top_layout(layoutI);

    for (size_t i = 0; i < mButton.size(); i++)
    {
        mButton[i].isEnabled = isEnabled[i];
        ui_push_button(&mButton[i], label[i]);
        if (ui_button_is_pressed())
            hasPressed = static_cast<int>(i + 1);
        ui_pop();
    }

    ui_pop();
    return hasPressed;
}

template class EUIButtonRow<2>;
template class EUIButtonRow<3>;

bool EUIAssetPathEditRow::update(AssetRegistry& assetReg, std::string& path)
{
    EditorTheme theme = eui_get_theme();
    UILayoutInfo layoutI = theme.make_text_row_layout();
    bool hasChanged = false;
    bool hasSubmitted = false;
    std::string collidingPath;

    Color fgColor = theme.get_ui_theme().get_on_surface_color();

    ui_push_text(nullptr, "Asset URI Path");
    ui_top_layout(layoutI);
    ui_pop();

    UITextEditWidget editW = ui_push_text_edit(&mEdit);
    if (!editW.is_editing())
        mEdit.set_text(path);
    hasChanged = ui_text_edit_changed(path);
    hasSubmitted = ui_text_edit_submitted(path);
    if (hasChanged && assetReg)
    {
        mIsPathValid = assetReg.is_path_valid(path, collidingPath);

        if (path.empty())
            mStatus.clear_value();
        else if (mIsPathValid)
        {
            mStatus.set_value("Valid path", &fgColor);
        }
        else if (!collidingPath.empty())
        {
            theme.get_error_color(fgColor);
            mStatus.set_value("Path collision with [" + collidingPath + "]", &fgColor);
        }
        else
        {
            theme.get_error_color(fgColor);
            mStatus.set_value("Invalid path", &fgColor);
        }
    }
    ui_top_layout(layoutI);
    ui_pop();

    ui_push_text(&mStatus);
    ui_pop();

    return hasSubmitted;
}

} // namespace LD
