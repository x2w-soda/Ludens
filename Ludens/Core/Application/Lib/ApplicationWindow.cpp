#include <GLFW/glfw3.h>
#include "Core/Application/Lib/ApplicationWindow.h"
#include "Core/Application/Include/Application.h"
#include "Core/Header/Include/Error.h"

namespace LD {

	void ApplicationWindow::Startup(const ApplicationWindowConfig& config, const Application* app)
	{
		mApp = app;
		mName = config.Name;
		mWidth = config.Width;
		mHeight = config.Height;

		int result = glfwInit();
		LD_DEBUG_ASSERT(result == GLFW_TRUE);

		if (!config.IsVisible)
			glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

		if (mApp->GetRendererBackend() == RBackend::OpenGL)
		{
			glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
			glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
			glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
		}
		else
			glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		
		mHandle = glfwCreateWindow((int)mWidth, (int)mHeight, mName.c_str(), nullptr, nullptr);
		LD_DEBUG_ASSERT(mHandle != nullptr);

		glfwSetWindowUserPointer(mHandle, this);

		glfwMakeContextCurrent(mHandle);
		glfwSwapInterval(config.EnableVsync ? 1 : 0);
		glfwGetFramebufferSize(mHandle, &mPixelWidth, &mPixelHeight);

		InstallCallbacks();

		mHasStartup = true;
	}

	void ApplicationWindow::Cleanup()
	{
		mHasStartup = false;

		glfwDestroyWindow(mHandle);
		glfwTerminate();
	}

	double ApplicationWindow::GetTime()
	{
		LD_DEBUG_ASSERT(mHasStartup);

		return glfwGetTime();
	}

	void ApplicationWindow::GetCursorPosition(float& screenX, float& screenY)
	{
		LD_DEBUG_ASSERT(mHasStartup);

		double x, y;
		glfwGetCursorPos(mHandle, &x, &y);
		screenX = (float)x;
		screenY = (float)y;
	}

	void* ApplicationWindow::GetHandle()
	{
		LD_DEBUG_ASSERT(mHasStartup && mHandle != nullptr);

		return (void*)mHandle;
	}

	void ApplicationWindow::PollEvents()
	{
		LD_DEBUG_ASSERT(mHasStartup);

		glfwPollEvents();
	}

	void ApplicationWindow::SwapBuffers()
	{
		LD_DEBUG_ASSERT(mHasStartup);

		// TODO: this is a graphics context operation, related to the image swap chain.
		//       currently we have GLFW manage our OpenGL context.
		glfwSwapBuffers(mHandle);
	}

	bool ApplicationWindow::IsAlive()
	{
		LD_DEBUG_ASSERT(mHasStartup);

		return !glfwWindowShouldClose(mHandle);
	}

	void ApplicationWindow::SetCursorHidden()
	{
		glfwSetInputMode(mHandle, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
	}

	void ApplicationWindow::SetCursorNormal()
	{
		glfwSetInputMode(mHandle, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
	}

	void ApplicationWindow::SetCursorGrabbed()
	{
		glfwSetInputMode(mHandle, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	}

	void ApplicationWindow::InstallCallbacks()
	{
		glfwSetWindowSizeCallback(mHandle, [](GLFWwindow* window, int width, int height) {
			ApplicationWindow* appWindow = (ApplicationWindow*)glfwGetWindowUserPointer(window);
			appWindow->mWidth = width;
			appWindow->mHeight = height;

			ApplicationWindowResizeEvent event;
			event.Width = width;
			event.Height = height;
			EventDispatch(event, &Application::EventHandler);
		});

		// The frame buffer size is measured in pixels, which may or may not equal the window size in screen coordinates.
		glfwSetFramebufferSizeCallback(mHandle, [](GLFWwindow* window, int width, int height) {
            ApplicationWindow* appWindow = (ApplicationWindow*)glfwGetWindowUserPointer(window);
			appWindow->mPixelWidth = width;
            appWindow->mPixelHeight = height;

			ApplicationFrameBufferResizeEvent event;
			event.PixelWidth = width;
			event.PixelHeight = height;
			EventDispatch(event, &Application::EventHandler);
		});

		glfwSetKeyCallback(mHandle, [](GLFWwindow* window, int key, int scancode, int action, int mods) {
			if (action == GLFW_PRESS || action == GLFW_REPEAT)
			{
				KeyPressedEvent event;
				event.Key = static_cast<KeyCode>(key);
				event.Repeat = action == GLFW_REPEAT;
				EventDispatch(event, &Application::EventHandler);
			}
			else if (action == GLFW_RELEASE)
			{
				KeyReleasedEvent event;
				event.Key = static_cast<KeyCode>(key);
				EventDispatch(event, &Application::EventHandler);
			}
		});

		glfwSetMouseButtonCallback(mHandle, [](GLFWwindow* window, int button, int action, int mods) {
			if (action == GLFW_PRESS)
			{
				MouseButtonPressedEvent event;
				event.Button = static_cast<MouseButton>(button);
				EventDispatch(event, &Application::EventHandler);
			}
			else if (action == GLFW_RELEASE)
			{
				MouseButtonReleasedEvent event;
				event.Button = static_cast<MouseButton>(button);
				EventDispatch(event, &Application::EventHandler);
			}
		});

		glfwSetCursorPosCallback(mHandle, [](GLFWwindow* window, double xpos, double ypos) {
			MouseMotionEvent event;
			event.XPos = (float)xpos;
			event.YPos = (float)ypos;
			EventDispatch(event, &Application::EventHandler);
		});

		glfwSetScrollCallback(mHandle, [](GLFWwindow* window, double xoffset, double yoffset) {
			MouseScrolledEvent event;
			event.XOffset = (float)xoffset;
			event.YOffset = (float)yoffset;
			EventDispatch(event, &Application::EventHandler);
		});
	}

} // namespace LD