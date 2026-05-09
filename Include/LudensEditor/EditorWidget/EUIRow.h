#pragma once

#include <Ludens/DSA/Array.h>
#include <Ludens/DSA/String.h>
#include <Ludens/UI/UIImmediate.h>

namespace LD {

struct AssetRegistry;
struct Project;
class UITextEditData;

void eui_row_label(const char* label);
bool eui_row_label_text_edit(const char* label, UITextEditData* edit, String& outText);
void eui_push_row_scroll(UIScrollData* scroll);
void eui_pop_row_scroll();

class EUILabelRow
{
public:
    bool update(const char* label, int rowIndex, bool isHighlighted, String& outNewLabel);

private:
    UITextEditData mLabel;
};

template <size_t TCount>
class EUIButtonRow
{
public:
    Array<const char*, TCount> label = {};
    Array<bool, TCount> isEnabled;
    UIAlign rowAlign = UI_ALIGN_END;

    EUIButtonRow()
    {
        std::fill(isEnabled.begin(), isEnabled.end(), true);
    }

    int update();

private:
    Array<UIButtonData, TCount> mButton;
};

class EUIAssetPathEditRow
{
public:
    bool update(AssetRegistry& assetReg, String& path);

    inline bool is_path_valid() const { return mIsPathValid; }

private:
    UITextEditData mEdit;
    UITextData mStatus;
    bool mIsPathValid = true;
};

class EUIScenePathEditRow
{
public:
    bool update(Project& project, String& path);

    inline bool is_path_valid() const { return mIsPathValid; }

private:
    UITextEditData mEdit;
    UITextData mStatus;
    bool mIsPathValid = true;
};

} // namespace LD
