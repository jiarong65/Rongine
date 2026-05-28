#include "Rongpch.h"
#include "Application.h"
#include "ApplicationEvent.h"
#include "Log.h"
#include "Platform/Windows/WindowsWindow.h"
#include "Input.h"
#include "Rongine/Renderer/Renderer.h"
#include "Rongine/Scene/SpectralAssetManager.h"
#include "Rongine/Renderer/Threading/RenderThread.h"

#include <GLFW/glfw3.h>
#include <chrono>
#include <thread>

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
		SpectralAssetManager::init();

		m_imguiLayer = new ImGuiLayer();
	}

	Application::~Application() {

	}

	void Application::onEvent(Event& event) {
		RONG_CORE_INFO( event.toString());
		EventDispatcher dispatcher(event);

		dispatcher.dispatch<WindowCloseEvent>(RONG_BIND_EVENT_FN(onWindowClose));
		dispatcher.dispatch<WindowResizeEvent>(RONG_BIND_EVENT_FN(onWindowResize));

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

	float Application::getTime()
	{
		return (float)glfwGetTime();
	}

	void Application::run() {
		WindowResizeEvent e(1280,720);
		RONG_CLIENT_TRACE( e.toString());

		// // Phase 1 smoke test: one magenta frame on the render thread
		// GLFWwindow* win = static_cast<GLFWwindow*>(m_window->getNativeWindow());

		// // The context was created/current on the main thread. GLFW requires it
		// // to be detached before another thread can make it current.
		// glfwMakeContextCurrent(nullptr);

		// RenderThread::start(win);
		// RenderThread::submit([win]() {
		// 	int width = 0;
		// 	int height = 0;
		// 	glfwGetFramebufferSize(win, &width, &height);
		// 	glViewport(0, 0, width, height);
		// 	glClearColor(1.0f, 0.0f, 1.0f, 1.0f);
		// 	glClear(GL_COLOR_BUFFER_BIT);
		// 	glfwSwapBuffers(win);
		// });
		// RenderThread::sync();

		// // Keep the frame visible while the main thread continues pumping events.
		// const double smokeTestEndTime = glfwGetTime() + 0.8;
		// while (glfwGetTime() < smokeTestEndTime)
		// {
		// 	glfwPollEvents();
		// 	std::this_thread::sleep_for(std::chrono::milliseconds(16));
		// }

		// RenderThread::shutdown();
		// glfwMakeContextCurrent(win);

		// RONG_CORE_INFO("Render-thread smoke test done (magenta frame). GL context is back on the main thread.");

		glfwMakeContextCurrent(nullptr);

		GLFWwindow* win=std::static_cast<GLFWwindow*>(m_window->getNativeWindow());
		RenderThread::start(win);

		RenderThread::submit([win](){
			Renderer::init();
			SpectralAssetManager::init();
		});

		RenderThread::sync();

		while (m_running) {
			float time = (float)glfwGetTime();
			Timestep ts = time - m_lastFrameTime;
			m_lastFrameTime = time;

			if (!m_minimized) {
				for (Layer* layer : m_layerStack)
					layer->onUpdate(ts);
			}

			m_imguiLayer->begin();
			for (Layer* layer : m_layerStack)
				layer->onImGuiRender();
			m_imguiLayer->end();

			m_window->onUpdate();
		}

		RenderThread::shutdown();
	}

	void Application::close()
	{
		m_running = false;
	}

	bool Application::onWindowClose(WindowCloseEvent& event){
		m_running = false;
		return false;
	}

	bool Application::onWindowResize(WindowResizeEvent& event) {
		if (event.getWidth() == 0 || event.getHeight() == 0)
		{
			m_minimized = true;
			return false;
		}

		m_minimized = false;
		Renderer::onWindowResize(event.getWidth(), event.getHeight());
		
		return false;
	}
}
