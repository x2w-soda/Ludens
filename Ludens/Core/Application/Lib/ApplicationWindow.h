#pragma once

#include <string>
#include "Core/Header/Include/Types.h"
#include "Core/Application/Include/Application.h"

class GLFWwindow;

namespace LD {

	class ApplicationWindow
	{
	public:
		ApplicationWindow() {}
		ApplicationWindow(const ApplicationWindow&) = delete;
		~ApplicationWindow() {}

		ApplicationWindow& operator=(const ApplicationWindow&) = delete;

		void Startup(const ApplicationWindowConfig& config, const Application* app);
		void Cleanup();

		// elapsed time in seconds since window Startup
		double GetTime();

		// mouse cursor position in screen space
		void GetCursorPosition(float& screenX, float& screenY);
		void* GetHandle();
		inline std::string GetName() const { return mName; }
		inline int GetWidth() const { return mWidth; }
		inline int GetHeight() const { return mHeight; }
        inline int GetPixelWidth() const { return mPixelWidth; }
		inline int GetPixelHeight() const { return mPixelHeight; }
		inline float GetAspectRatio() const { return (float)mWidth / (float)mHeight; }
		
		void PollEvents();
		void SwapBuffers();
		bool IsAlive();

		void SetCursorHidden();
		void SetCursorNormal();
		void SetCursorGrabbed();

	private:
		void InstallCallbacks();

		const Application* mApp = nullptr;
		std::string mName;
		int mWidth = 0;
		int mHeight = 0;
        int mPixelWidth = 0;
        int mPixelHeight = 0;
		GLFWwindow* mHandle = nullptr;
		bool mHasStartup = false;
	};

}; // namespace LD