#pragma once

#include "Core/Header/Include/Types.h"
#include "Core/Application/Include/Input.h"

namespace LD {

	struct Event;
	typedef bool (*EventHandlerFn)(const Event& event);

	bool EventDispatch(Event& event, EventHandlerFn handler);

	enum class EventType
	{
		None = 0,
		ApplicationQuit,
		ApplicationWindowResize,
		ApplicationFrameBufferResize,
		ApplicationMinimized,
		KeyPressed,
		KeyReleased,
		MouseMotion,
		MouseScrolled,
		MouseButtonPressed,
		MouseButtonReleased,
	};

	using EventFlags = unsigned char;
	enum EventFlagBits
	{
		EVENT_FLAGS_NONE = 0,
		EVENT_FLAGS_APPLICATION_BIT = 1<<0,
		EVENT_FLAGS_INPUT_BIT = 1<<1,
		EVENT_FLAGS_INPUT_KEY_BIT = 1<<2,
		EVENT_FLAGS_INPUT_MOUSE_BIT = 1<<3,
	};

	struct Event
	{
		Event(const Event&) = default;
		Event(EventType type, EventFlags flags)
			: Type(type), Flags(flags), IsHandled(false) {}
		virtual ~Event() {}

		Event& operator=(const Event&) = default;

		inline bool IsApplicationEvent() const { return (Flags & EVENT_FLAGS_APPLICATION_BIT) == EVENT_FLAGS_APPLICATION_BIT; }
		inline bool IsInputEvent() const { return (Flags & EVENT_FLAGS_INPUT_BIT) == EVENT_FLAGS_INPUT_BIT; }

		const EventType Type = EventType::None;
		const EventFlags Flags = EVENT_FLAGS_NONE;
		bool IsHandled;
	};

	struct ApplicationQuitEvent : Event
	{
		ApplicationQuitEvent()
			: Event(EventType::ApplicationQuit, EVENT_FLAGS_APPLICATION_BIT) {};
	};

	struct ApplicationWindowResizeEvent : Event
	{
		ApplicationWindowResizeEvent()
			: Event(EventType::ApplicationWindowResize, EVENT_FLAGS_APPLICATION_BIT) {};

		int Width;    // window width in screen coordinates
		int Height;   // window height in screen coordinates
	};

	struct ApplicationFrameBufferResizeEvent : Event
	{
		ApplicationFrameBufferResizeEvent()
			: Event(EventType::ApplicationFrameBufferResize, EVENT_FLAGS_APPLICATION_BIT) {};

		int PixelWidth;    // window framebuffer width in pixels
		int PixelHeight;   // window framebuffer height in pixels
	};

	struct ApplicationMinimizedEvent : Event
	{
        ApplicationMinimizedEvent()
			: Event(EventType::ApplicationMinimized, EVENT_FLAGS_APPLICATION_BIT){};
	};

	struct KeyPressedEvent : Event
	{
		KeyPressedEvent()
			: Event(EventType::KeyPressed, EVENT_FLAGS_INPUT_BIT | EVENT_FLAGS_INPUT_KEY_BIT) {};

		KeyCode Key;
		bool Repeat;
	};

	struct KeyReleasedEvent : Event
	{
		KeyReleasedEvent()
			: Event(EventType::KeyReleased, EVENT_FLAGS_INPUT_BIT | EVENT_FLAGS_INPUT_KEY_BIT) {};

		KeyCode Key;
	};

	struct MouseMotionEvent : Event
	{
		MouseMotionEvent()
			: Event(EventType::MouseMotion, EVENT_FLAGS_INPUT_BIT | EVENT_FLAGS_INPUT_MOUSE_BIT) {}

		float XPos;
		float YPos;
	};

	struct MouseScrolledEvent : Event
	{
		MouseScrolledEvent()
			: Event(EventType::MouseScrolled, EVENT_FLAGS_INPUT_BIT | EVENT_FLAGS_INPUT_MOUSE_BIT) {}

		float XOffset;
		float YOffset;
	};

	struct MouseButtonPressedEvent : Event
	{
		MouseButtonPressedEvent()
			: Event(EventType::MouseButtonPressed, EVENT_FLAGS_INPUT_BIT | EVENT_FLAGS_INPUT_MOUSE_BIT) {};

		MouseButton Button;
	};

	struct MouseButtonReleasedEvent : Event
	{
		MouseButtonReleasedEvent()
			: Event(EventType::MouseButtonReleased, EVENT_FLAGS_INPUT_BIT | EVENT_FLAGS_INPUT_MOUSE_BIT) {};

		MouseButton Button;
	};

} // namespace LD