#pragma once

#include "Core.h"
#include "Rongine/Window.h"
#include "Rongine/Events/Event.h"
#include "Rongine/Events/ApplicationEvent.h"
#include "Rongine/Events/KeyEvent.h"
#include "Rongine/Events/MouseEvent.h"

namespace Rongine {
	class RONG_API  Application
	{
	public:
		Application();
		virtual ~Application();
		void run();

		void onEvent(Event& event);
	private:
		bool onWindowResize(WindowResizeEvent& event);
		bool onWindowClose(WindowCloseEvent& event);
	private:
		std::unique_ptr<Window> m_window;
		bool m_running = true;
	};

	Application* createApplication();
}


