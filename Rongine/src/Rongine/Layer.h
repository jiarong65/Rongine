#pragma once

#include "Rongine/Events/Event.h"
#include "Rongine/Core.h"
#include <string>

namespace Rongine {

	class RONG_API Layer
	{
	public:
		Layer(const std::string& name = "Layer");
		virtual ~Layer() = default;

		virtual void onAttach() {};
		virtual void onDetach() {};
		virtual void onUpdate() {};
		virtual void onImGuiRender() {};
		virtual void onEvent(Event& event) {};

		const std::string& getName() { return m_debugName; }
	protected:
		std::string m_debugName;
	};

}

