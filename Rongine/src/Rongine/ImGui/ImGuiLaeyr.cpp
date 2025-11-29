#include "Rongpch.h"
#include "ImGuiLaeyr.h"
#include "imgui.h"
#include "Platform/OpenGL/ImGuiOpenGLRenderer.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "Rongine/Application.h"

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
		ImGui::CreateContext();
		ImGui::StyleColorsDark();

		ImGuiIO& io = ImGui::GetIO();
		io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;
		io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;
		io.KeyMap[ImGuiKey_Tab] = GLFW_KEY_TAB;
		io.KeyMap[ImGuiKey_LeftArrow] = GLFW_KEY_LEFT;
		io.KeyMap[ImGuiKey_RightArrow] = GLFW_KEY_RIGHT;
		io.KeyMap[ImGuiKey_UpArrow] = GLFW_KEY_UP;
		io.KeyMap[ImGuiKey_DownArrow] = GLFW_KEY_DOWN;
		io.KeyMap[ImGuiKey_PageUp] = GLFW_KEY_PAGE_UP;
		io.KeyMap[ImGuiKey_PageDown] = GLFW_KEY_PAGE_DOWN;
		io.KeyMap[ImGuiKey_Home] = GLFW_KEY_HOME;
		io.KeyMap[ImGuiKey_End] = GLFW_KEY_END;
		io.KeyMap[ImGuiKey_Insert] = GLFW_KEY_INSERT;
		io.KeyMap[ImGuiKey_Delete] = GLFW_KEY_DELETE;
		io.KeyMap[ImGuiKey_Backspace] = GLFW_KEY_BACKSPACE;
		io.KeyMap[ImGuiKey_Space] = GLFW_KEY_SPACE;
		io.KeyMap[ImGuiKey_Enter] = GLFW_KEY_ENTER;
		io.KeyMap[ImGuiKey_Escape] = GLFW_KEY_ESCAPE;
		io.KeyMap[ImGuiKey_KeyPadEnter] = GLFW_KEY_KP_ENTER;
		io.KeyMap[ImGuiKey_A] = GLFW_KEY_A;
		io.KeyMap[ImGuiKey_C] = GLFW_KEY_C;
		io.KeyMap[ImGuiKey_V] = GLFW_KEY_V;
		io.KeyMap[ImGuiKey_X] = GLFW_KEY_X;
		io.KeyMap[ImGuiKey_Y] = GLFW_KEY_Y;
		io.KeyMap[ImGuiKey_Z] = GLFW_KEY_Z;

		ImGui_ImplOpenGL3_Init("#version 410");
	}

	void ImGuiLayer::onDetach()
	{

	}

	void ImGuiLayer::onUpdate()
	{
		float time = (float)glfwGetTime();
		float deltatime = time - m_time;

		ImGuiIO& io = ImGui::GetIO();
		io.DeltaTime = m_time ? deltatime : (1.f / 60.f);
		m_time = time;

		Application& app = Application::get();
		io.DisplaySize = ImVec2(app.getWindow().getWidth(), app.getWindow().getHeight());


		ImGui_ImplOpenGL3_NewFrame();
		ImGui::NewFrame();

		static bool show = true;
		ImGui::ShowDemoWindow(&show);

		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
	}

	void ImGuiLayer::onEvent(Event& event)
	{
		EventDispatcher dispatcher(event);
		dispatcher.dispatch<MouseButtonPressedEvent>(RONG_BIND_EVENT_FN(ImGuiLayer::onMouseButtonPressedEvent));
		dispatcher.dispatch<MouseButtonReleasedEvent>(RONG_BIND_EVENT_FN(ImGuiLayer::onMouseButtonReleasedEvent));
		dispatcher.dispatch<MouseMovedEvent>(RONG_BIND_EVENT_FN(ImGuiLayer::onMouseMovedEvent));
		dispatcher.dispatch<MouseScrolledEvent>(RONG_BIND_EVENT_FN(ImGuiLayer::onMouseScrolledEvent));
		dispatcher.dispatch<KeyPressedEvent>(RONG_BIND_EVENT_FN(ImGuiLayer::onKeyPressedEvent));
		dispatcher.dispatch<KeyReleasedEvent>(RONG_BIND_EVENT_FN(ImGuiLayer::onKeyReleasedEvent));
		dispatcher.dispatch<KeyTypedEvent>(RONG_BIND_EVENT_FN(ImGuiLayer::onKeyTypedEvent));
		dispatcher.dispatch<WindowResizeEvent>(RONG_BIND_EVENT_FN(ImGuiLayer::onWindowResizedEvent));
	}

	bool ImGuiLayer::onMouseButtonPressedEvent(MouseButtonPressedEvent& event)
	{
		int button = event.getMouseButton();
		
		ImGuiIO& io = ImGui::GetIO();
		if(button>=0&&button<3)
			io.MouseDown[button] = true;

		return io.WantCaptureMouse;
	}

	bool ImGuiLayer::onMouseButtonReleasedEvent(MouseButtonReleasedEvent& event)
	{
		int button = event.getMouseButton();

		ImGuiIO& io = ImGui::GetIO();
		if (button >= 0 && button < 3)
			io.MouseDown[button] = false;

		return io.WantCaptureMouse;
	}

	bool ImGuiLayer::onMouseMovedEvent(MouseMovedEvent& event)
	{
		ImGuiIO& io = ImGui::GetIO();
		io.MousePos = ImVec2(event.getX(), event.getY());

		return io.WantCaptureMouse;
	}

	bool ImGuiLayer::onMouseScrolledEvent(MouseScrolledEvent& event)
	{
		ImGuiIO& io = ImGui::GetIO();
		io.MouseWheel += event.getYOffset();
		io.MouseWheelH += event.getXOffset();
		return io.WantCaptureMouse;
	}

	bool ImGuiLayer::onKeyPressedEvent(KeyPressedEvent& event)
	{
		ImGuiIO& io = ImGui::GetIO();
		io.KeysDown[event.getKeyCode()] = true;

		io.KeyAlt = io.KeysDown[GLFW_KEY_LEFT_ALT] || io.KeysDown[GLFW_KEY_RIGHT_ALT];
		io.KeyCtrl = io.KeysDown[GLFW_KEY_LEFT_CONTROL] || io.KeysDown[GLFW_KEY_RIGHT_CONTROL];
		io.KeyShift = io.KeysDown[GLFW_KEY_LEFT_SHIFT] || io.KeysDown[GLFW_KEY_RIGHT_SHIFT];
		io.KeySuper = io.KeysDown[GLFW_KEY_LEFT_SUPER] || io.KeysDown[GLFW_KEY_RIGHT_SUPER];
		return io.WantCaptureKeyboard;
	}

	bool ImGuiLayer::onKeyReleasedEvent(KeyReleasedEvent& event)
	{
		ImGuiIO& io = ImGui::GetIO();
		io.KeysDown[event.getKeyCode()] = false;

		io.KeyAlt = io.KeysDown[GLFW_KEY_LEFT_ALT] || io.KeysDown[GLFW_KEY_RIGHT_ALT];
		io.KeyCtrl = io.KeysDown[GLFW_KEY_LEFT_CONTROL] || io.KeysDown[GLFW_KEY_RIGHT_CONTROL];
		io.KeyShift = io.KeysDown[GLFW_KEY_LEFT_SHIFT] || io.KeysDown[GLFW_KEY_RIGHT_SHIFT];
		io.KeySuper = io.KeysDown[GLFW_KEY_LEFT_SUPER] || io.KeysDown[GLFW_KEY_RIGHT_SUPER];
		return io.WantCaptureKeyboard;
	}

	bool ImGuiLayer::onKeyTypedEvent(KeyTypedEvent& event)
	{
		ImGuiIO& io = ImGui::GetIO();
		unsigned int keycode = event.getKeyCode();

		// 只有当字符是可打印字符时才添加到 ImGui 的文本缓冲区
		if (keycode > 0 && keycode < 0x10000)
			io.AddInputCharacter((unsigned short)keycode);

		return io.WantCaptureKeyboard;
	}

	bool ImGuiLayer::onWindowResizedEvent(WindowResizeEvent& event)
	{
		ImGuiIO& io = ImGui::GetIO();
		if (event.getWidth() > 0 && event.getHeight() > 0) {
			io.DisplaySize = ImVec2((float)event.getWidth(), (float)event.getHeight());
			io.DisplayFramebufferScale = ImVec2(1.f, 1.f);
			glViewport(0, 0, event.getWidth(), event.getHeight());
		}

		return false;
	}

}