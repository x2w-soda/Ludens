#include "Core/Application/Include/Event.h"

namespace LD {

	bool EventDispatch(Event& event, EventHandlerFn handler)
	{
		// NOTE: synchronous dispatch for all types of events,
		//       blocks until event handler returns
		return event.IsHandled = handler(event);
	}

} // namespace LD