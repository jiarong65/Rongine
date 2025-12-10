#include "Rongpch.h"
#include "Application.h"
#include "ApplicationEvent.h"
#include "Log.h"
#include "Platform/Windows/WindowsWindow.h"
#include "Platform/Windows/WindowsInput.h"
#include "Input.h"
#include "Rongine/Renderer/Renderer.h"

namespace Rongine {
	Application* Application::s_instance = nullptr;

	Application::Application() 
	{
		RONG_CORE_ASSERT(!s_instance, "Application already exists!");
		s_instance = this;
		m_window = std::unique_ptr<Window> (Window::create());
		m_window->setEventCallBack(RONG_BIND_EVENT_FN(onEvent));
		m_window->setVSync(true);

		Renderer::init();

		m_imguiLayer = new ImGuiLayer();
	}

	Application::~Application() {

	}

	void Application::onEvent(Event& event) {
		RONG_CORE_INFO( event.toString());
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
	}

	void Application::pushOverLayer(Layer* layer)
	{
		m_layerStack.pushOverLayer(layer);
	}

	void Application::run() {
		WindowResizeEvent e(1280,720);
		RONG_CLIENT_TRACE( e.toString());

		while (m_running) {
			float time = (float)glfwGetTime();
			Timestep ts = time - m_lastFrameTime;
			m_lastFrameTime = time;

			for (Layer* layer : m_layerStack)
				layer->onUpdate(ts);

			m_imguiLayer->begin();
			for (Layer* layer : m_layerStack)
				layer->onImGuiRender();
			m_imguiLayer->end();

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
