#include "Rongpch.h"
#include "ImGuiLayer.h"
#include "imgui.h"
#include "examples/imgui_impl_glfw.h"
#include "examples/imgui_impl_opengl3.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "Rongine/Core/Application.h"

namespace Rongine {
	ImGuiLayer::ImGuiLayer()
		:Layer("ImGuiLayer")
	{
	}

	ImGuiLayer::~ImGuiLayer()
	{
	}

	void ImGuiLayer::onAttach()
	{
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGui::StyleColorsDark();

		ImGuiIO& io = ImGui::GetIO();
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // Enable Docking
		// 多视口会在渲染线程里创建额外 GLFW 窗口，主线程 sync 阻塞时易死锁/无响应
		// io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

		io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;
		io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;

		ImGuiStyle& style =ImGui::GetStyle();
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			style.WindowRounding = 0.0f;
			style.Colors[ImGuiCol_WindowBg].w = 1.0f;
		}

		GLFWwindow* window = static_cast<GLFWwindow*>(Application::get().getWindow().getNativeWindow());
		ImGui_ImplGlfw_InitForOpenGL(window, true);

		// 多线程需要将opengl上下文迁移到渲染线程
		//ImGui_ImplOpenGL3_Init("#version 410");
	}

	void ImGuiLayer::onDetach()
	{
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();
	}

	void ImGuiLayer::onEvent(Event& e)
	{
		if (m_blockEvents)
		{
			ImGuiIO& io = ImGui::GetIO();
			if (e.isInCateGory(EventCategoryMouse))
			{
				// 如果 e.handled 变成 true，事件就不会传给下层的 EditorLayer
				e.handled |= io.WantCaptureMouse;
			}

			if (e.isInCateGory(EventCategoryKeyboard))
			{
				e.handled |= io.WantCaptureKeyboard;
			}
		}
	}

	void ImGuiLayer::updateRendererFrame()
	{
		RONG_CORE_ASSERT(m_openGLBackendInitialized, "OpenGL ImGui backend not initialized");
		ImGui_ImplOpenGL3_NewFrame();
	}

	void ImGuiLayer::updatePlatformInput()
	{
		RONG_CORE_ASSERT(m_openGLBackendInitialized, "OpenGL ImGui backend not initialized");
		// Win32：glfwGet* / 光标须在创建窗口的线程（主线程）调用
		ImGui_ImplGlfw_NewFrame();
	}

	void ImGuiLayer::begin()
	{
		ImGui::NewFrame();
	}

	void ImGuiLayer::end()
	{
		auto& app=Application::get();
		ImGuiIO& io = ImGui::GetIO();
		const uint32_t width = app.getWindow().getWidth();
		const uint32_t height = app.getWindow().getHeight();
		io.DisplaySize = ImVec2((float)width, (float)height);

		glViewport(0, 0, width, height);
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			GLFWwindow* backup_current_context = glfwGetCurrentContext();
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
			glfwMakeContextCurrent(backup_current_context);
		}
	}

	void ImGuiLayer::initOpenGLBackent()
	{
		ImGui_ImplOpenGL3_Init("#version 410");
		m_openGLBackendInitialized = true;
		// 在渲染线程上构建 Font Atlas（ImGui_ImplGlfw_NewFrame 依赖 IsBuilt）
		ImGui_ImplOpenGL3_NewFrame();
	}

	void ImGuiLayer::shutdownOpenGLBackend()
	{
		if(!m_openGLBackendInitialized)
			return;
		ImGui_ImplOpenGL3_Shutdown();
		m_openGLBackendInitialized=false;
	}

}
