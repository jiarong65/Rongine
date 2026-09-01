module;

#include <memory>
#include <string>
#include <utility>
#include <cstdint>
#include <GLFW/glfw3.h>
#include "Rongine/Core/RongineMacros.h"

module Rongine.Application;

import Rongine.Core;
import Rongine.Log;
import Rongine.LayerStack;
import Rongine.Events;
import Rongine.Window;
import Rongine.ImGuiLayer;
import Rongine.Renderer;
import Rongine.RenderThread;
import Rongine.SpectralAssetManager;

namespace Rongine {

	Application::Application()
	{
		RONG_CORE_ASSERT(!s_instance, "Application already exists!");
		s_instance = this;

		m_window = std::unique_ptr<Window>(Window::create());
		m_window->setEventCallBack(RONG_BIND_EVENT_FN(onEvent));
		m_window->setVSync(true);

		//Renderer::init();
		//SpectralAssetManager::init();

		m_imguiLayer = new ImGuiLayer();
		m_layerStack.setDeferAttach(true);
	}

	void Application::run()
	{
		WindowResizeEvent e(1280, 720);
		RONG_CLIENT_TRACE(e.toString());

		// ImGui GLFW 回调须在主线程安装（Windows 要求窗口 API 在主线程）
		static_cast<ImGuiLayer*>(m_imguiLayer)->attachIfNeeded();

		glfwMakeContextCurrent(nullptr);

		GLFWwindow* win = static_cast<GLFWwindow*>(m_window->getNativeWindow());
		RenderThread::start(win);

		RenderThread::submit([this]() {
			Renderer::init();
			SpectralAssetManager::init();
			m_layerStack.attachAll();
			static_cast<ImGuiLayer*>(m_imguiLayer)->initOpenGLBackent();
		});

		RenderThread::sync();
		RONG_CORE_INFO("Render thread init done.");

		while (m_running)
		{
			float time = (float)glfwGetTime();
			Timestep ts = time - m_lastFrameTime;
			m_lastFrameTime = time;

			m_window->pollEvents();

			// 标题栏 ×：GLFW 置 shouldClose 并触发 WindowCloseEvent；此处与回调双保险
			if (glfwWindowShouldClose(win))
				m_running = false;

			// ImGui：OpenGL3 在渲染线程；GLFW 平台输入在主线程（须在 pollEvents 之后）
			if (static_cast<ImGuiLayer*>(m_imguiLayer)->isOpenGLBackendReady())
			{
				RenderThread::submit([this]() {
					static_cast<ImGuiLayer*>(m_imguiLayer)->updateRendererFrame();
				});
				RenderThread::sync();
				static_cast<ImGuiLayer*>(m_imguiLayer)->updatePlatformInput();
			}

			if (!m_minimized)
			{
				for (Layer* layer : m_layerStack)
					layer->onUpdate(ts);
			}

			RenderThread::submit([this, ts]() {
				static_cast<ImGuiLayer*>(m_imguiLayer)->begin();
				for (Layer* layer : m_layerStack)
					layer->onImGuiRender();
				static_cast<ImGuiLayer*>(m_imguiLayer)->end();

				m_window->swapBuffers();
			});

			RenderThread::sync();
		}

		RenderThread::submit([this]() {
			// 先释放编辑器 GL 资源，再关 ImGui OpenGL / GLFW
			for (Layer* layer : m_layerStack)
			{
				if (layer != static_cast<ImGuiLayer*>(m_imguiLayer))
					layer->detachIfNeeded();
			}

			static_cast<ImGuiLayer*>(m_imguiLayer)->shutdownOpenGLBackend();
			static_cast<ImGuiLayer*>(m_imguiLayer)->detachIfNeeded();
		});

		RenderThread::sync();
		RenderThread::shutdown();

		// 销毁窗口前必须释放 GL 上下文，否则 glfwDestroyWindow 可能 0xc0000005
		glfwMakeContextCurrent(nullptr);
	}
}
