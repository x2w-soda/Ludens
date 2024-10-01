#pragma once

#include "Core/Application/Include/Event.h"
#include "Core/OS/Include/Time.h"

namespace LD {

	class Application;

	class ApplicationLayer
	{
	public:
		ApplicationLayer() {}
		ApplicationLayer(const ApplicationLayer&) = delete;
		virtual ~ApplicationLayer() {}

		ApplicationLayer& operator=(const ApplicationLayer&) = delete;

		virtual void OnAttach(const Application& app) {}
		virtual void OnDetach(const Application& app) {}
		virtual bool OnEvent(const Event& event) { return false; }

		// NOTE: temporary solution before multi-threading and job systems
		//       - Fixed Update is called on a fixed interval defined by driver Application,
		//         this update method is preferable for physics simulations.
		//       - Delta Update may have varying delta times for each call,
		//         this update method is used by most subsystems without physics
		//       - Fixed update and Delta update should not have inter-dependencies, if both are overriden
		virtual void OnFixedUpdate(DeltaTime dt) {}
		virtual void OnDeltaUpdate(DeltaTime dt) {}

	private:
	};

} // namespace LD