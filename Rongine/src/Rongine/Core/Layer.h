#pragma once

#include "Rongine/Events/Event.h"
#include "Rongine/Core/Core.h"
#include "Rongine/Core/Timestep.h"

namespace Rongine {

	class RONG_API Layer
	{
	public:
		Layer(const std::string& name = "Layer");
		virtual ~Layer() = default;

		virtual void onAttach() {};
		virtual void onDetach() {};
		virtual void onUpdate(Timestep ts) {};
		virtual void onImGuiRender() {};
		virtual void onEvent(Event& e) {};

		const std::string& getName() { return m_debugName; }
	protected:
		std::string m_debugName;
	};

}

