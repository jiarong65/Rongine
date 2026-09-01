module;

#include <chrono>
#include <cstdint>
#include <memory>
#include <thread>
#include <utility>
#include <vector>

#include "Rongine/Core/RongineMacros.h"
#include <GLFW/glfw3.h>

import Rongine.Renderer;

export module Rongine.Application;

export import Rongine.Core;
export import Rongine.Log;
export import Rongine.LayerStack;
export import Rongine.Events;
export import Rongine.RendererCameras;
export import Rongine.RenderThread;
export import Rongine.SpectralAssetManager;
export import Rongine.Window;

export namespace Rongine {

	class RONG_API Application
	{
	public:
		virtual ~Application();
		void run();
		void close();

		void* getImGuiLayer() { return m_imguiLayer; }

		void onEvent(Event& event);

		void pushLayer(Layer* layer);
		void pushOverLayer(Layer* layer);

		float getTime();

		inline Window& getWindow() { return *m_window; }
		inline static Application& get() { return *s_instance; }
	protected:
		Application();
	private:
		bool onWindowResize(WindowResizeEvent& event);
		bool onWindowClose(WindowCloseEvent& event);
	private:
		static Application* s_instance;
		std::unique_ptr<Window> m_window;

		bool m_running = true;
		bool m_minimized = false;
		LayerStack m_layerStack;
		void* m_imguiLayer;

		float m_lastFrameTime = 0.0f;
	};

	Application* Application::s_instance = nullptr;

	Application::~Application()
	{
	}

	void Application::onEvent(Event& event)
	{
		RONG_CORE_INFO(event.toString());

		EventDispatcher dispatcher(event);
		dispatcher.dispatch<WindowCloseEvent>(RONG_BIND_EVENT_FN(onWindowClose));
		dispatcher.dispatch<WindowResizeEvent>(RONG_BIND_EVENT_FN(onWindowResize));

		for (auto it = m_layerStack.rbegin(); it != m_layerStack.rend(); ++it)
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

	float Application::getTime()
	{
		return (float)glfwGetTime();
	}

	void Application::close()
	{
		m_running = false;
	}

	bool Application::onWindowClose(WindowCloseEvent& event)
	{
		m_running = false;
		return false;
	}

	bool Application::onWindowResize(WindowResizeEvent& event)
	{
		if (event.getWidth() == 0 || event.getHeight() == 0)
		{
			m_minimized = true;
			return false;
		}

		m_minimized = false;

		const uint32_t width = event.getWidth();
		const uint32_t height = event.getHeight();
		RenderThread::submit([width, height]() {
			Renderer::onWindowResize(width, height);
		});

		return false;
	}
}
