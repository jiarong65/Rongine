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

		void begin();
		void end();

	private:
		float m_time = 0.0f;
	};

}

