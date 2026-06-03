#include "Rongpch.h"

#include "Application.h"

#include "ApplicationEvent.h"

#include "Log.h"

#include "Platform/Windows/WindowsWindow.h"

#include "Input.h"

#include "Rongine/Renderer/Renderer.h"

#include "Rongine/Renderer/Renderer3D.h"

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



		//Renderer::init();

		//SpectralAssetManager::init();



		m_imguiLayer = new ImGuiLayer();

		m_layerStack.setDeferAttach(true);

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



		// ImGui GLFW 回调须在主线程安装（Windows 要求窗口 API 在主线程）

		m_imguiLayer->attachIfNeeded();



		glfwMakeContextCurrent(nullptr);



		GLFWwindow* win = static_cast<GLFWwindow*>(m_window->getNativeWindow());

		RenderThread::start(win);



		RenderThread::submit([this](){

			Renderer::init();

			SpectralAssetManager::init();

			m_layerStack.attachAll();

			m_imguiLayer->initOpenGLBackent();

		});



		RenderThread::sync();

		RONG_CORE_INFO("Render thread init done.");



		while (m_running) {

			float time = (float)glfwGetTime();

			Timestep ts = time - m_lastFrameTime;

			m_lastFrameTime = time;



			m_window->pollEvents();



			// 标题栏 ×：GLFW 置 shouldClose 并触发 WindowCloseEvent；此处与回调双保险

			if (glfwWindowShouldClose(win))

				m_running = false;



			// ImGui：OpenGL3 在渲染线程；GLFW 平台输入在主线程（须在 pollEvents 之后）

			if (m_imguiLayer->isOpenGLBackendReady())

			{

				RenderThread::submit([this]() {

					m_imguiLayer->updateRendererFrame();

				});

				RenderThread::sync();

				m_imguiLayer->updatePlatformInput();

			}



			if(!m_minimized)

			{

				for(Layer* layer : m_layerStack)

					layer->onUpdate(ts);

			}



			RenderThread::submit([this, ts](){

				m_imguiLayer->begin();

				for (Layer* layer : m_layerStack)

					layer->onImGuiRender();

				m_imguiLayer->end();



				m_window->swapBuffers();

			});



			RenderThread::sync();

		}



		RenderThread::submit([this]() {
			// 先释放编辑器 GL 资源，再关 ImGui OpenGL / GLFW
			for (Layer* layer : m_layerStack)
			{
				if (layer != m_imguiLayer)
					layer->detachIfNeeded();
			}

			m_imguiLayer->shutdownOpenGLBackend();
			m_imguiLayer->detachIfNeeded();
		});

		RenderThread::sync();

	

		RenderThread::shutdown();

		// 销毁窗口前必须释放 GL 上下文，否则 glfwDestroyWindow 可能 0xc0000005

		glfwMakeContextCurrent(nullptr);

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



		const uint32_t width = event.getWidth();

		const uint32_t height = event.getHeight();

		RenderThread::submit([width, height]() {

			Renderer::onWindowResize(width, height);

		});



		return false;

	}

}

