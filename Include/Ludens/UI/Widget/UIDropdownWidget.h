#pragma once

#include <Ludens/DSA/Vector.h>
#include <Ludens/UI/UIDef.h>
#include <Ludens/UI/UIWidget.h>

#include <string>

namespace LD {

typedef void (*UIDropdownOnOpen)(UIWidget dropdown);

class UIDropdownData
{
    friend struct UIDropdownWidgetObj;

public:
    UIDropdownOnOpen onOpen = nullptr;
    Vector<std::string> options;
    int popupLevel = 0;
    float fontSize = UIFont::base_size();

private:
    int mOptionIndex = 0;
};

struct UIDropdownWidget : UIWidget
{
    bool set_option(int option);
};

} // namespace LD