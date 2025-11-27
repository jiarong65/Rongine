#include "Rongpch.h"
#include "Application.h"
#include "ApplicationEvent.h"
#include "Log.h"
#include "Platform/Windows/WindowsWindow.h"

namespace Rongine {
	Application::Application() {
		m_window = std::unique_ptr<Window> (Window::create());
		m_window->setEventCallBack(RONG_BIND_EVENT_FN(onEvent));
	}

	Application::~Application() {

	}

	void Application::onEvent(Event& event) {
		//RONG_CORE_INFO( event.toString());
		EventDispatcher dispatcher(event);

		dispatcher.dispatch<WindowCloseEvent>(RONG_BIND_EVENT_FN(onWindowClose));

		for (auto& it = m_layerStack.rbegin(); it != m_layerStack.rend();++it)
		{
			if (event.handled)
				break;
			(*it)->onEvent(event);
		}
	}

	void Application::pushLayer(Layer* layer)
	{
		m_layerStack.pushLayer(layer);
		layer->onAttach();
	}

	void Application::pushOverLayer(Layer* layer)
	{
		m_layerStack.pushOverLayer(layer);
		layer->onAttach();
	}

	void Application::run() {
		WindowResizeEvent e(1280,720);
		RONG_CLIENT_TRACE( e.toString());

		while (m_running) {
			glClearColor(1,0,1,1);
			glClear(GL_COLOR_BUFFER_BIT);
			m_window->onUpdate();

			for (Layer* layer : m_layerStack)
				layer->onUpdate();
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
