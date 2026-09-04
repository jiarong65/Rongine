module;

#include <cstdint>

#include "Rongine/Core/RongineMacros.h"
#include "imgui.h"
#include "examples/imgui_impl_glfw.h"
#include "examples/imgui_impl_opengl3.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>

export module Rongine.ImGuiLayer;

export import Rongine.Core;
export import Rongine.LayerStack;
export import Rongine.Events;
import Rongine.Application;
import Rongine.Renderer;

export namespace Rongine {
	class RONG_API ImGuiLayer :public Layer
	{
	public:
		ImGuiLayer();
		~ImGuiLayer();

		virtual void onAttach() override;
		virtual void onDetach() override;

		virtual void onEvent(Event& e) override;

		// 渲染线程：OpenGL3 NewFrame（构建/更新 Font Atlas）
		void updateRendererFrame();
		// 主线程：pollEvents 之后调用（GLFW 输入/显示尺寸，须在 updateRendererFrame 之后）
		void updatePlatformInput();

		void begin();
		void end();

		void initOpenGLBackent();
		void shutdownOpenGLBackend();
		bool isOpenGLBackendReady() const { return m_openGLBackendInitialized; }

		void blockEvents(bool block) { m_blockEvents = block; }
		bool getBlockEvents() const { return m_blockEvents; }
	private:
		float m_time = 0.0f;
		bool m_blockEvents = false;
		bool m_openGLBackendInitialized = false;
	};

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
		// 不启用 NavEnableKeyboard：启用后按 Alt 会激活 ImGui 菜单层抢焦点
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

		io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;
		io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;

		ImGuiStyle& style = ImGui::GetStyle();
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			style.WindowRounding = 0.0f;
			style.Colors[ImGuiCol_WindowBg].w = 1.0f;
		}

		GLFWwindow* window = static_cast<GLFWwindow*>(Application::get().getWindow().getNativeWindow());
		ImGui_ImplGlfw_InitForOpenGL(window, true);
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
		ImGui_ImplGlfw_NewFrame();
	}

	void ImGuiLayer::begin()
	{
		ImGui::NewFrame();
	}

	void ImGuiLayer::end()
	{
		auto& app = Application::get();
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
		ImGui_ImplOpenGL3_NewFrame();
	}

	void ImGuiLayer::shutdownOpenGLBackend()
	{
		if (!m_openGLBackendInitialized)
			return;
		ImGui_ImplOpenGL3_Shutdown();
		m_openGLBackendInitialized = false;
	}
}
