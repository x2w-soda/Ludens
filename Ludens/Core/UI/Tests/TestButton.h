#pragma once

#include <doctest.h>
#include "Core/UI/Include/UI.h"
#include "Core/UI/Include/Control/Control.h"

using namespace LD;

static bool sFlag = false;

static void Click(UIContext* ctx, UIButton* btn)
{
	sFlag = true;
}

TEST_CASE("UIButton Click On Press")
{
	UIContext context;
	UIWindow window;
	UIButton button;

	UIContextInfo info{};
	info.Width = 1600.0f;
	info.Height = 900.0f;
	context.Startup(info);

	UIWindowInfo windowI{};
    windowI.Context = &context;
	windowI.Parent = context.GetRoot();
	windowI.Rect = { 0.0f, 0.0f, 600.0f, 500.0f };
	window.Startup(windowI);
	{
		UIButtonInfo buttonI{};
		buttonI.Widget.Parent = &window;
		buttonI.Widget.Width = 200.0f;
		buttonI.Widget.Height = 200.0f;
		buttonI.ClickMode = UIClickMode::ClickOnPress;
		buttonI.OnClick = Click;
		button.Startup(buttonI);

		context.BeginFrame(0.16f);

		Rect2D rect = button.GetRect();
		printf("button: (%.2f,%.2f), (%.2f, %.2f)\n", rect.x, rect.y, rect.w, rect.h);

		sFlag = false;

		context.InputMousePosition({ 50.0f, 50.0f });
		context.InputMouseButtonPressed(MOUSE_BUTTON_LEFT);

		CHECK(sFlag);

		context.InputMouseButtonReleased(MOUSE_BUTTON_LEFT);

		context.EndFrame();
		button.Cleanup();
	}
	window.Cleanup();
	context.Cleanup();
}