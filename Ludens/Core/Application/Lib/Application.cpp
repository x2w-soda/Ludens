#include "Core/Application/Include/Application.h"
#include "Core/Application/Include/ApplicationLayer.h"
#include "Core/Application/Lib/ApplicationWindow.h"
#include "Core/Header/Include/Error.h"
#include "Core/OS/Include/Time.h"
#include "Core/OS/Include/Memory.h"


namespace LD {

	// singleton instance
	Application* Application::sInstance = nullptr;


	Application::Application()
	{
	}

	Application::~Application()
	{
	}

	void Application::Startup(const ApplicationConfig& config)
	{
		LD_DEBUG_ASSERT(config.Layer != nullptr);

		mConfig = config;

		mWindow = new ApplicationWindow();
		mWindow->Startup(mConfig.Window, this);
		mLayer = config.Layer;
		mLayer->OnAttach(*this);

		mHasStartup = true;
	}

	void Application::Cleanup()
	{
		mHasStartup = false;

		mLayer->OnDetach(*this);
		mWindow->Cleanup();
		delete mWindow;
		mWindow = nullptr;
	}

	void* Application::GetWindowHandle() const
	{
		return static_cast<void*>(mWindow->GetHandle());
	}

	void Application::GetWindowSize(int* width, int* height) const
	{
		if (width)
			*width = mWindow->GetWidth();
		if (height)
			*height = mWindow->GetHeight();
	}

	void Application::GetWindowPixelSize(int* width, int* height) const
	{
        if (width)
            *width = mWindow->GetPixelWidth();
        if (height)
            *height = mWindow->GetPixelHeight();
	}


	void Application::SetWindowCursorNormal()
	{
		mWindow->SetCursorNormal();
	}

	void Application::SetWindowCursorGrabbed()
	{
		mWindow->SetCursorGrabbed();
	}

	bool Application::EventHandler(const Event& event)
	{
		Application& app = Application::GetSingleton();

		if (event.IsApplicationEvent())
		{
			switch (event.Type)
			{
			case EventType::ApplicationQuit:
			{
				app.mIsRunning = false;
				return true; // consume event
			}
			case EventType::ApplicationWindowResize:
			{
				auto e = static_cast<const ApplicationWindowResizeEvent&>(event);
				app.mIsMinimized = (e.Width == 0 || e.Height == 0);
				
				// immediately flush the minimization event
				if (app.mIsMinimized)
				{
                    EventDispatch(ApplicationMinimizedEvent{}, &Application::EventHandler);
				}

				break;
			}
			default:
				break;
			}
		}
		
		if (event.IsInputEvent())
		{
			app.OnInputEvent(event);

			// Input events will be forwarded to layers, and they can choose
			// to act on events or directly or poll the input state via Input.h
		}

		if (!app.mLayer)
			return false;

		return app.mLayer->OnEvent(event);
	}

	void Application::Run(const ApplicationConfig& config)
	{
        Startup(config);

		// NOTE: currently uses the platform's window system timer
		//       which may or may not be the OS clock
		double timeLastFrame;
		double timeThisFrame = mWindow->GetTime();
		double fixedUpdateTimer = 0.0f;

		mIsRunning = true;
		while (mIsRunning && mWindow->IsAlive())
		{
			timeLastFrame = timeThisFrame;
			timeThisFrame = mWindow->GetTime();
			DeltaTime dt(timeThisFrame - timeLastFrame);

			OnInputNewFrame();

			mWindow->PollEvents();

			fixedUpdateTimer += dt.GetSeconds();
			if (fixedUpdateTimer > mConfig.FixedUpdateInterval)
			{
				fixedUpdateTimer -= mConfig.FixedUpdateInterval;
				mLayer->OnFixedUpdate({ mConfig.FixedUpdateInterval });
			}
			
			mLayer->OnDeltaUpdate(dt);

			mWindow->SwapBuffers();
		}

		Cleanup();
	}

} // namespace LD
