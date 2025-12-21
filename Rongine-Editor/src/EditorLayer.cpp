#include "Rongpch.h"
#include <Rongine.h>
#include "EditorLayer.h"
#include "Platform/OpenGL/OpenGLShader.h"
#include "Rongine/Core/Input.h"
#include "Rongine/Core/KeyCodes.h"
#include <glm/gtc/type_ptr.hpp>
#include <chrono>

#include "imgui/imgui.h"

template<typename Fn>
class Timer
{
public:
	Timer(const char* name, Fn&& func)
		:m_name(name), m_func(func),m_stopped(false)
	{
		m_startTimepoint = std::chrono::high_resolution_clock::now();
	}

	~Timer()
	{
		if (!m_stopped)
			stop();
	}

	void stop()
	{
		auto endTimepoint = std::chrono::high_resolution_clock::now();

		long long start = std::chrono::time_point_cast<std::chrono::microseconds>(m_startTimepoint).time_since_epoch().count();
		long long end = std::chrono::time_point_cast<std::chrono::microseconds>(endTimepoint).time_since_epoch().count();

		float duration = (end - start)*0.001;
		m_func({ m_name,duration });
	}
private:
	const char* m_name;
	Fn m_func;
	std::chrono::time_point<std::chrono::steady_clock> m_startTimepoint;
	bool m_stopped;
};

#define PROFILE_SCOPE(name) Timer timer##__LINE__(name,[this](ProfileResult result){m_profileResult.push_back(result);});


EditorLayer::EditorLayer()
	:Layer("EditorLayer"), m_cameraContorller(1280.0f/720.0f)
{
}

void EditorLayer::onAttach()
{
	m_checkerboardTexture = Rongine::Texture2D::create("assets/textures/Checkerboard.png");
	m_logoTexture = Rongine::Texture2D::create("assets/textures/ChernoLogo.png");

	Rongine::FrameSpecification fbSpec;
	fbSpec.width = 1280;
	fbSpec.height = 720;
	m_framebuffer = Rongine::Framebuffer::create(fbSpec);
}

void EditorLayer::onDetach()
{
}

void EditorLayer::onUpdate(Rongine::Timestep ts)
{
	PROFILE_SCOPE("EditorLayer::OnUpdate");

	{
		PROFILE_SCOPE("CameraContorller::OnUpdate");
		m_cameraContorller.onUpdate(ts);
	}

	{
		PROFILE_SCOPE("EditorLayer::Input");
		if (Rongine::Input::isKeyPressed(Rongine::Key::J))
			m_squarePosition.x -= m_squareMovedSpeed * ts;
		else if (Rongine::Input::isKeyPressed(Rongine::Key::L))
			m_squarePosition.x += m_squareMovedSpeed * ts;
		if (Rongine::Input::isKeyPressed(Rongine::Key::I))
			m_squarePosition.y += m_squareMovedSpeed * ts;
		else if (Rongine::Input::isKeyPressed(Rongine::Key::K))
			m_squarePosition.y -= m_squareMovedSpeed * ts;
	}

	{
		PROFILE_SCOPE("Renderer Prep");

		m_framebuffer->bind();

		Rongine::RenderCommand::setColor({ 0.1f, 0.1f, 0.1f, 1 });
		Rongine::RenderCommand::clear();
	}
	

	Rongine::Renderer2D::resetStatistics();
	{
		static float rotation = 0.0f;
		rotation += ts * 50.0f;

		PROFILE_SCOPE("Renderer Draw");
		Rongine::Renderer2D::beginScene(m_cameraContorller.getCamera());
		Rongine::Renderer2D::drawRotatedQuad({ 1.0f, 0.0f }, { 0.8f, 0.8f }, -45.0f, { 0.8f, 0.2f, 0.3f, 1.0f });
		Rongine::Renderer2D::drawQuad({ 0.0f, 0.0f, -0.1f }, { 20.0f, 20.0f }, m_checkerboardTexture,10.0f,glm::vec4(1.0f,0.8f,0.8f,0.5f));
		//Rongine::Renderer2D::drawRotatedQuad(m_squarePosition, { 1.0f, 1.0f }, 60.0f,m_squareColor);
		Rongine::Renderer2D::drawQuad({ 0.5f, -0.5f }, { 0.5f, 0.75f }, { 0.2f, 0.3f, 0.8f, 1.0f });
		Rongine::Renderer2D::drawQuad(m_squarePosition, { 1.0f, 1.0f },  m_squareColor);
		Rongine::Renderer2D::drawRotatedQuad({ -2.0f, 0.0f, 1.0f }, { 1.0f, 1.0f }, rotation, m_logoTexture, 20.0f);

		for (float x = -5.0f; x < 5.0f; x += 0.5f) {
			for (float y = -5.0f; y < 5.0f; y += 0.5f) {
				glm::vec4 color = { (x + 5.0f) / 10.0f,0.4f,(y + 5.0f) / 10.0f,0.7f };
				Rongine::Renderer2D::drawQuad({ x, y }, { 0.45f, 0.45f }, color);
			}
		}

		Rongine::Renderer2D::endScene();

		m_framebuffer->unbind();
	}

}

//void EditorLayer::onImGuiRender()
//{
//	ImGui::Begin("Setting");
//	ImGui::ColorEdit3("Square Color", glm::value_ptr(m_squareColor));
//
//	auto stats = Rongine::Renderer2D::getStatistics();
//	ImGui::Text("Renderer2D Stats:");
//	ImGui::Text("Draw Calls: %d", stats.DrawCalls);
//	ImGui::Text("Quads: %d", stats.QuadCounts);
//	ImGui::Text("Vertices: %d", stats.getTotalVertexCounts());
//	ImGui::Text("Indices: %d", stats.getTotalIndexCounts());
//
//	for (auto& result : m_profileResult)
//	{
//		char label[50];
//		strcpy(label, "%.3fms  ");
//		strcat(label, result.name);
//		ImGui::Text(label, result.time);
//	}
//	m_profileResult.clear();
//	ImGui::End();
//}

void EditorLayer::onImGuiRender()
{
	// --- DockSpace 核心开始 ---
	static bool dockspaceOpen = true;
	static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

	// 设置主窗口标志（全屏、无标题、不可移动等）
	ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
	ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(viewport->Pos);
	ImGui::SetNextWindowSize(viewport->Size);
	ImGui::SetNextWindowViewport(viewport->ID);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
	window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

	// 创建底层的 DockSpace 宿主窗口
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
	ImGui::Begin("DockSpace Demo", &dockspaceOpen, window_flags);
	ImGui::PopStyleVar();
	ImGui::PopStyleVar(2); // 弹出之前 Push 的两个 StyleVar

	// 激活 DockSpace
	ImGuiIO& io = ImGui::GetIO();
	if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
	{
		ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
		ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
	}

	// 顶部菜单栏
	if (ImGui::BeginMenuBar())
	{
		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::MenuItem("Exit")) Rongine::Application::get().close();
			ImGui::EndMenu();
		}
		ImGui::EndMenuBar();
	}
	// --- DockSpace 核心结束 ---

	// 真正的 Settings 窗口（现在它可以被停靠到任何地方）
	ImGui::Begin("Settings");

	ImGui::ColorEdit4("Square Color", glm::value_ptr(m_squareColor));

	auto stats = Rongine::Renderer2D::getStatistics();
	ImGui::Text("Renderer2D Stats:");
	ImGui::Text("Draw Calls: %d", stats.DrawCalls);
	ImGui::Text("Quads: %d", stats.QuadCounts);
	ImGui::Text("Vertices: %d", stats.getTotalVertexCounts());
	ImGui::Text("Indices: %d", stats.getTotalIndexCounts());

	// 渲染性能监控
	ImGui::Separator();
	for (auto& result : m_profileResult)
	{
		ImGui::Text("%.3fms  %s", result.time, result.name);
	}
	m_profileResult.clear();

	// 模拟视口预览（将纹理渲染在窗口里）
	uint32_t textureID = m_checkerboardTexture->getRendererID();
	ImGui::Image((void*)(uintptr_t)textureID, ImVec2{ 256.0f, 256.0f }, { 0, 1 }, { 1, 0 });

	ImGui::End(); // 结束 Settings 窗口

	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 0, 0 }); // 让图片铺满窗口
	ImGui::Begin("Viewport");

	// 拿到 FBO 里的纹理 ID
	textureID = m_framebuffer->getColorAttachmentRendererID();
	// 将 FBO 的内容绘制到 ImGui 窗口中
	// 注意：OpenGL 的 UV 坐标是 y 轴向上，ImGui 是向下，所以需要翻转 UV：{0, 1}, {1, 0}
	ImGui::Image((void*)(uintptr_t)textureID, ImVec2{ 1280, 720 }, ImVec2{ 0, 1 }, ImVec2{ 1, 0 });

	ImGui::End();
	ImGui::PopStyleVar();

	ImGui::End(); // 结束 DockSpace 宿主窗口
}

void EditorLayer::onEvent(Rongine::Event& e)
{
	m_cameraContorller.onEvent(e);
}
