#include "Rongpch.h"
#include "EditorLayer.h"
#include "Platform/OpenGL/OpenGLShader.h"
#include "Rongine/Core/Input.h"
#include "Rongine/Core/KeyCodes.h"
// 必须包含 Renderer3D 头文件
#include "Rongine/Renderer/Renderer3D.h" 
#include <glm/gtc/type_ptr.hpp>
#include <chrono>

#include "imgui/imgui.h"

// --- Timer 类保持不变 ---
template<typename Fn>
class Timer
{
public:
	Timer(const char* name, Fn&& func)
		:m_name(name), m_func(func), m_stopped(false)
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
		float duration = (end - start) * 0.001f;
		m_func({ m_name, duration });
	}
private:
	const char* m_name;
	Fn m_func;
	std::chrono::time_point<std::chrono::steady_clock> m_startTimepoint;
	bool m_stopped;
};

#define PROFILE_SCOPE(name) Timer timer##__LINE__(name,[this](ProfileResult result){m_profileResult.push_back(result);});

EditorLayer::EditorLayer()
	:Layer("EditorLayer"), m_cameraContorller(45.0f, 1280.0f / 720.0f)
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

	// --- 【关键修复】初始化 Renderer3D ---
	Rongine::Renderer3D::init();
}

void EditorLayer::onDetach()
{
	// --- 【推荐】关闭 Renderer3D ---
	Rongine::Renderer3D::shutdown();
}

void EditorLayer::onUpdate(Rongine::Timestep ts)
{
	PROFILE_SCOPE("EditorLayer::OnUpdate");

	// 1. 调整相机大小 (如果视口变了)
	if (m_viewportSize.x > 0.0f && m_viewportSize.y > 0.0f)
	{
		m_cameraContorller.onResize(m_viewportSize.x, m_viewportSize.y);
	}

	// 2. 更新相机逻辑
	{
		PROFILE_SCOPE("CameraContorller::OnUpdate");
		if (m_viewportFocused)
			m_cameraContorller.onUpdate(ts);
	}

	// 3. 准备渲染 (清屏)
	{
		PROFILE_SCOPE("Renderer Prep");
		m_framebuffer->bind();
		Rongine::RenderCommand::setColor({ 0.1f, 0.1f, 0.1f, 1 });
		// 确保 Clear 包含 GL_DEPTH_BUFFER_BIT
		Rongine::RenderCommand::clear();
	}

	// 4. 3D 渲染压力测试
	{
		PROFILE_SCOPE("Renderer 3D");

		Rongine::Renderer3D::beginScene(m_cameraContorller.getCamera());

		// --- 场景 1: 地板 ---
		// 绘制一个带有棋盘格纹理的大地板
		Rongine::Renderer3D::drawCube({ 0.0f, -0.1f, 0.0f }, { 20.0f, 0.1f, 20.0f }, m_checkerboardTexture);

		// --- 场景 2: 压力测试 (20x20 = 400个方块) ---
		// 验证批处理是否生效：如果 DrawCalls 只有 1 或 2，说明成功了。
		for (int x = -10; x < 10; x++)
		{
			for (int z = -10; z < 10; z++)
			{
				glm::vec3 pos = { x * 0.6f, 1.0f, z * 0.6f };
				glm::vec3 scale = { 0.5f, 0.5f, 0.5f };

				// 根据位置生成渐变色
				glm::vec4 color = { (x + 10) / 20.0f, 0.4f, (z + 10) / 20.0f, 0.8f };

				Rongine::Renderer3D::drawCube(pos, scale, color);
			}
		}

		// 画一个 Logo 测试透明度和混合
		Rongine::Renderer3D::drawCube({ 0.0f, 2.5f, 0.0f }, { 2.0f, 2.0f, 2.0f }, m_logoTexture);

		Rongine::Renderer3D::endScene();
	}

	// 5. 2D 渲染 (暂时注释掉以专注于测试 3D 性能，防止混淆)
	/*
	Rongine::Renderer2D::resetStatistics();
	{
		Rongine::Renderer2D::beginScene(m_cameraContorller.getCamera()); // 注意：Renderer2D 需要适配 PerspectiveCamera
		// ... 你的 2D 绘制代码 ...
		Rongine::Renderer2D::endScene();
	}
	*/

	m_framebuffer->unbind();
}

void EditorLayer::onImGuiRender()
{
	// --- DockSpace 核心开始 ---
	static bool dockspaceOpen = true;
	static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

	ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
	ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(viewport->Pos);
	ImGui::SetNextWindowSize(viewport->Size);
	ImGui::SetNextWindowViewport(viewport->ID);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
	window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
	ImGui::Begin("DockSpace Demo", &dockspaceOpen, window_flags);
	ImGui::PopStyleVar();
	ImGui::PopStyleVar(2);

	ImGuiIO& io = ImGui::GetIO();
	if (Rongine::Input::isKeyPressed(Rongine::Key::LeftAlt) ||
		Rongine::Input::isMouseButtonPressed(Rongine::Mouse::ButtonRight))
	{
		io.ConfigFlags &= ~ImGuiConfigFlags_NavEnableKeyboard;
	}
	else
	{
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	}

	if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
	{
		ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
		ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
	}

	if (ImGui::BeginMenuBar())
	{
		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::MenuItem("Exit")) Rongine::Application::get().close();
			ImGui::EndMenu();
		}
		ImGui::EndMenuBar();
	}

	// --- Settings 面板 ---
	ImGui::Begin("Settings");

	ImGui::ColorEdit4("Square Color", glm::value_ptr(m_squareColor));

	// 显示 3D 统计信息 (确保你在 Renderer3D 中实现了 getStatistics)
	auto stats = Rongine::Renderer3D::getStatistics();
	ImGui::Text("Renderer3D Stats:");
	ImGui::Text("Draw Calls: %d", stats.DrawCalls);
	ImGui::Text("Cubes: %d", stats.CubeCount);
	ImGui::Text("Vertices: %d", stats.GetTotalVertexCount());
	ImGui::Text("Indices: %d", stats.GetTotalIndexCount());

	ImGui::Separator();
	for (auto& result : m_profileResult)
	{
		ImGui::Text("%.3fms  %s", result.time, result.name);
	}
	m_profileResult.clear();

	// 重置统计
	Rongine::Renderer3D::resetStatistics();

	ImGui::End();

	// --- Viewport 面板 ---
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 0, 0 });
	ImGui::Begin("Viewport");

	m_viewportFocused = ImGui::IsWindowFocused();
	m_viewportHovered = ImGui::IsWindowHovered();
	Rongine::Application::get().getImGuiLayer()->blockEvents(!m_viewportFocused && !m_viewportHovered);

	// 1. 获取当前 ImGui 视口窗口的可用大小
	ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();

	// 2. 检查是否需要 Resize
	// 只有当尺寸发生变化，且尺寸有效时才调整
	if (m_viewportSize != *((glm::vec2*)&viewportPanelSize) && viewportPanelSize.x > 0 && viewportPanelSize.y > 0)
	{
		m_viewportSize = { viewportPanelSize.x, viewportPanelSize.y };
		// 调整 FBO 大小
		m_framebuffer->resize((uint32_t)m_viewportSize.x, (uint32_t)m_viewportSize.y);
		// 相机调整逻辑移动到了 OnUpdate 开头，或者在这里调用也可以
		m_cameraContorller.onResize(m_viewportSize.x, m_viewportSize.y);
	}

	uint32_t textureID = m_framebuffer->getColorAttachmentRendererID();

	// 3. 绘制图片 (使用动态的 m_viewportSize)
	ImGui::Image((void*)(uintptr_t)textureID, ImVec2{ m_viewportSize.x, m_viewportSize.y }, ImVec2{ 0, 1 }, ImVec2{ 1, 0 });

	ImGui::End();
	ImGui::PopStyleVar();

	ImGui::End(); // End DockSpace
}

void EditorLayer::onEvent(Rongine::Event& e)
{
	m_cameraContorller.onEvent(e);
}