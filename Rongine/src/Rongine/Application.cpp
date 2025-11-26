#include "Rongpch.h"
#include "Application.h"
#include "ApplicationEvent.h"
#include "Log.h"


namespace Rongine {
	Application::Application() {
		m_window = std::unique_ptr<Window> (Window::create());
		m_window->setEventCallBack(RONG_BIND_EVENT_FN(onEvent));
	}

	Application::~Application() {

	}

	void Application::onEvent(Event& event) {
		RONG_CORE_INFO( event.toString());
		EventDispatcher dispatcher(event);

		dispatcher.dispatch<WindowCloseEvent>(RONG_BIND_EVENT_FN(onWindowClose));
	}

	void Application::run() {
		WindowResizeEvent e(1280,720);
		RONG_CLIENT_TRACE( e.toString());

		while (m_running) {
			m_window->onUpdate();
		}
	}

	bool Application::onWindowClose(WindowCloseEvent& event){
		m_running = false;
		return true;
	}

	bool Application::onWindowResize(WindowResizeEvent& event) {
		return true;
	}
}
