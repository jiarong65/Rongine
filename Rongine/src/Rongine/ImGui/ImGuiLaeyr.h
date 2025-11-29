#pragma once
#include "Rongine/Layer.h"
#include "Rongine/Events/KeyEvent.h"
#include "Rongine/Events/MouseEvent.h"
#include "Rongine/Events/ApplicationEvent.h"

namespace Rongine {
	class RONG_API ImGuiLayer :public Layer
	{
	public:
		ImGuiLayer();
		~ImGuiLayer();

		void onAttach() override;
		void onDetach();
		void onUpdate();
		void onEvent(Event& event);
	private:
		bool onMouseButtonPressedEvent(MouseButtonPressedEvent& event);
		bool onMouseButtonReleasedEvent(MouseButtonReleasedEvent& event);
		bool onMouseMovedEvent(MouseMovedEvent& event);
		bool onMouseScrolledEvent(MouseScrolledEvent& event);
		bool onKeyPressedEvent(KeyPressedEvent& event);
		bool onKeyReleasedEvent(KeyReleasedEvent& event);
		bool onKeyTypedEvent(KeyTypedEvent& event);
		bool onWindowResizedEvent(WindowResizeEvent& event);

	private:
		float m_time = 0.0f;
	};

}

