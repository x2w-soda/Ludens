#pragma once

#include <Ludens/UI/UIWindow.h>

namespace LD {

struct TopBarOption;

/// @brief Editor top bar menu UI.
class EditorTopBar
{
public:
    void startup(UIWindow root);
    void cleanup();

    void draw_overlay(ScreenRenderComponent renderer);

private:
    UIWindow mRoot;
    TopBarOption* mFileOption;
    TopBarOption* mHelpOption;
    float mTopBarHeight;
};

} // namespace LD