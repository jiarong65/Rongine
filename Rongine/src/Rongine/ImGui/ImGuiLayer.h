#pragma once
#include "Rongine/Core/Layer.h"
#include "Rongine/Events/KeyEvent.h"
#include "Rongine/Events/MouseEvent.h"
#include "Rongine/Events/ApplicationEvent.h"

namespace Rongine {
	class RONG_API ImGuiLayer :public Layer
	{
	public:
		ImGuiLayer();
		~ImGuiLayer();

		virtual void onAttach() override;
		virtual void onDetach() override;

		virtual void onEvent(Event& e) override;

		void begin();
		void end();

		void blockEvents(bool block) { m_blockEvents = block; }
		bool getBlockEvents() const { return m_blockEvents; }
	private:
		float m_time = 0.0f;
		bool m_blockEvents = false;	
	};

}

