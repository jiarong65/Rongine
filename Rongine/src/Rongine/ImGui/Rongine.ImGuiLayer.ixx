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

// =============================================================
// 非导出：编辑器深色主题
// 配色体系取自 dougbinks 深色变体（dear-imgui-styles 合集收录）：
// 蓝灰深色底 + #4296fa 蓝色强调 + 圆角控件，按 ImGui 1.79 可用字段适配
// =============================================================
namespace Rongine {

	static void applyRongineDarkTheme()
	{
		ImGuiStyle& style = ImGui::GetStyle();
		ImVec4* c = style.Colors;

		// 基底（蓝灰深色）
		c[ImGuiCol_Text]                 = ImVec4(0.92f, 0.94f, 0.97f, 1.00f);
		c[ImGuiCol_TextDisabled]         = ImVec4(0.50f, 0.55f, 0.61f, 1.00f);
		c[ImGuiCol_WindowBg]             = ImVec4(0.11f, 0.12f, 0.14f, 1.00f);
		c[ImGuiCol_ChildBg]              = ImVec4(0.13f, 0.14f, 0.17f, 1.00f);
		c[ImGuiCol_PopupBg]              = ImVec4(0.13f, 0.14f, 0.17f, 0.98f);
		c[ImGuiCol_Border]               = ImVec4(0.28f, 0.32f, 0.38f, 0.50f);
		c[ImGuiCol_BorderShadow]         = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);

		// 输入框
		c[ImGuiCol_FrameBg]              = ImVec4(0.18f, 0.20f, 0.24f, 1.00f);
		c[ImGuiCol_FrameBgHovered]       = ImVec4(0.26f, 0.59f, 0.98f, 0.20f);
		c[ImGuiCol_FrameBgActive]        = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);

		// 标题栏 / 菜单栏
		c[ImGuiCol_TitleBg]              = ImVec4(0.09f, 0.10f, 0.12f, 1.00f);
		c[ImGuiCol_TitleBgActive]        = ImVec4(0.15f, 0.19f, 0.26f, 1.00f);
		c[ImGuiCol_TitleBgCollapsed]     = ImVec4(0.09f, 0.10f, 0.12f, 0.75f);
		c[ImGuiCol_MenuBarBg]            = ImVec4(0.14f, 0.16f, 0.19f, 1.00f);

		// 滚动条
		c[ImGuiCol_ScrollbarBg]          = ImVec4(0.10f, 0.11f, 0.13f, 0.60f);
		c[ImGuiCol_ScrollbarGrab]        = ImVec4(0.31f, 0.35f, 0.41f, 1.00f);
		c[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.40f, 0.45f, 0.52f, 1.00f);
		c[ImGuiCol_ScrollbarGrabActive]  = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);

		// 控件强调色（蓝）
		c[ImGuiCol_CheckMark]            = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
		c[ImGuiCol_SliderGrab]           = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
		c[ImGuiCol_SliderGrabActive]     = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);

		// 按钮
		c[ImGuiCol_Button]               = ImVec4(0.21f, 0.25f, 0.31f, 1.00f);
		c[ImGuiCol_ButtonHovered]        = ImVec4(0.24f, 0.34f, 0.48f, 1.00f);
		c[ImGuiCol_ButtonActive]         = ImVec4(0.26f, 0.40f, 0.60f, 1.00f);

		// 选中行 / 折叠标题
		c[ImGuiCol_Header]               = ImVec4(0.26f, 0.59f, 0.98f, 0.15f);
		c[ImGuiCol_HeaderHovered]        = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
		c[ImGuiCol_HeaderActive]         = ImVec4(0.26f, 0.59f, 0.98f, 0.70f);

		// 分隔线 / 窗口缩放把手
		c[ImGuiCol_Separator]            = ImVec4(0.28f, 0.32f, 0.38f, 0.55f);
		c[ImGuiCol_SeparatorHovered]     = ImVec4(0.26f, 0.59f, 0.98f, 0.78f);
		c[ImGuiCol_SeparatorActive]      = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
		c[ImGuiCol_ResizeGrip]           = ImVec4(0.26f, 0.59f, 0.98f, 0.10f);
		c[ImGuiCol_ResizeGripHovered]    = ImVec4(0.26f, 0.59f, 0.98f, 0.55f);
		c[ImGuiCol_ResizeGripActive]     = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);

		// Tab / Docking
		c[ImGuiCol_Tab]                  = ImVec4(0.14f, 0.16f, 0.19f, 1.00f);
		c[ImGuiCol_TabHovered]           = ImVec4(0.26f, 0.59f, 0.98f, 0.60f);
		c[ImGuiCol_TabActive]            = ImVec4(0.19f, 0.23f, 0.29f, 1.00f);
		c[ImGuiCol_TabUnfocused]         = ImVec4(0.12f, 0.13f, 0.16f, 1.00f);
		c[ImGuiCol_TabUnfocusedActive]   = ImVec4(0.17f, 0.20f, 0.25f, 1.00f);
		c[ImGuiCol_DockingPreview]       = ImVec4(0.26f, 0.59f, 0.98f, 0.70f);
		c[ImGuiCol_DockingEmptyBg]       = ImVec4(0.10f, 0.11f, 0.13f, 1.00f);

		// 图表 / 拖放 / 其他
		c[ImGuiCol_PlotLines]            = ImVec4(0.61f, 0.64f, 0.68f, 1.00f);
		c[ImGuiCol_PlotLinesHovered]     = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
		c[ImGuiCol_PlotHistogram]        = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
		c[ImGuiCol_PlotHistogramHovered] = ImVec4(0.46f, 0.72f, 1.00f, 1.00f);
		c[ImGuiCol_TextSelectedBg]       = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
		c[ImGuiCol_DragDropTarget]       = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
		c[ImGuiCol_NavHighlight]         = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
		c[ImGuiCol_ModalWindowDimBg]     = ImVec4(0.00f, 0.00f, 0.00f, 0.55f);

		// 形状与间距
		style.WindowPadding            = ImVec2(8.0f, 8.0f);
		style.FramePadding             = ImVec2(6.0f, 4.0f);
		style.ItemSpacing              = ImVec2(8.0f, 6.0f);
		style.ItemInnerSpacing         = ImVec2(6.0f, 4.0f);
		style.IndentSpacing            = 20.0f;
		style.ScrollbarSize            = 12.0f;
		style.WindowRounding           = 4.0f;
		style.ChildRounding            = 4.0f;
		style.PopupRounding            = 4.0f;
		style.FrameRounding            = 4.0f;
		style.TabRounding              = 5.0f;
		style.ScrollbarRounding        = 9.0f;
		style.GrabRounding             = 4.0f;
		style.WindowBorderSize         = 0.0f;
		style.ChildBorderSize          = 1.0f;
		style.PopupBorderSize          = 1.0f;
		style.FrameBorderSize          = 0.0f;
		style.WindowTitleAlign         = ImVec2(0.5f, 0.5f);
		style.WindowMenuButtonPosition = ImGuiDir_None;
	}

} // namespace Rongine

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
		applyRongineDarkTheme();

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
