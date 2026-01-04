#include "Rongpch.h"

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
}

void EditorLayer::onDetach()
{
}

void EditorLayer::onUpdate(Rongine::Timestep ts)
{
	PROFILE_SCOPE("EditorLayer::OnUpdate");

	{
		PROFILE_SCOPE("CameraContorller::OnUpdate");
		if(m_viewportFocused)
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

	{
		PROFILE_SCOPE("Renderer 3D");

		Rongine::Renderer3D::beginScene(m_cameraContorller.getCamera());

		// 绘制中心红块
		Rongine::Renderer3D::drawCube({ 0.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, { 0.8f, 0.2f, 0.3f, 1.0f });

		// 绘制一个“地面” (扁平的大块)
		Rongine::Renderer3D::drawCube({ 0.0f, -1.0f, 0.0f }, { 10.0f, 0.1f, 10.0f }, { 0.3f, 0.3f, 0.3f, 1.0f });

		Rongine::Renderer3D::endScene();
	}
	

	Rongine::Renderer2D::resetStatistics();
	{
		//static float rotation = 0.0f;
		//rotation += ts * 50.0f;

		//PROFILE_SCOPE("Renderer Draw");
		//Rongine::Renderer2D::beginScene(m_cameraContorller.getCamera());
		//Rongine::Renderer2D::drawRotatedQuad({ 1.0f, 0.0f }, { 0.8f, 0.8f }, -45.0f, { 0.8f, 0.2f, 0.3f, 1.0f });
		//Rongine::Renderer2D::drawQuad({ 0.0f, 0.0f, -0.1f }, { 20.0f, 20.0f }, m_checkerboardTexture,10.0f,glm::vec4(1.0f,0.8f,0.8f,0.5f));
		////Rongine::Renderer2D::drawRotatedQuad(m_squarePosition, { 1.0f, 1.0f }, 60.0f,m_squareColor);
		//Rongine::Renderer2D::drawQuad({ 0.5f, -0.5f }, { 0.5f, 0.75f }, { 0.2f, 0.3f, 0.8f, 1.0f });
		//Rongine::Renderer2D::drawQuad(m_squarePosition, { 1.0f, 1.0f },  m_squareColor);
		//Rongine::Renderer2D::drawRotatedQuad({ -2.0f, 0.0f, 1.0f }, { 1.0f, 1.0f }, rotation, m_logoTexture, 20.0f);

		//for (float x = -5.0f; x < 5.0f; x += 0.5f) {
		//	for (float y = -5.0f; y < 5.0f; y += 0.5f) {
		//		glm::vec4 color = { (x + 5.0f) / 10.0f,0.4f,(y + 5.0f) / 10.0f,0.7f };
		//		Rongine::Renderer2D::drawQuad({ x, y }, { 0.45f, 0.45f }, color);
		//	}
		//}

		//Rongine::Renderer2D::endScene();

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

//onImGuiRender

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
	if (Rongine::Input::isKeyPressed(Rongine::Key::LeftAlt) ||
		Rongine::Input::isMouseButtonPressed(Rongine::Mouse::ButtonRight))
	{
		// 移除 键盘导航 标志位
		io.ConfigFlags &= ~ImGuiConfigFlags_NavEnableKeyboard;
	}
	else
	{
		// 恢复 键盘导航 标志位 
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	}

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

	m_viewportFocused = ImGui::IsWindowFocused();
	m_viewportHovered = ImGui::IsWindowHovered();
	Rongine::Application::get().getImGuiLayer()->blockEvents(!m_viewportFocused && !m_viewportHovered);

	// 1. 获取当前 ImGui 视口窗口的可用大小
	ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();

	// 2. 检查是否需要 Resize
	// 如果尺寸变了，且尺寸有效（大于0）
	if (m_viewportSize != *((glm::vec2*)&viewportPanelSize) && viewportPanelSize.x > 0 && viewportPanelSize.y > 0)
	{
		m_viewportSize = { viewportPanelSize.x, viewportPanelSize.y };

		// A. 调整 Framebuffer 大小 (重新生成纹理附件)
		// 注意：你需要确保你的 Framebuffer 类实现了 resize() 方法
		m_framebuffer->resize((uint32_t)m_viewportSize.x, (uint32_t)m_viewportSize.y);

		// B. 调整相机宽高比 (防止物体被拉伸变形)
		m_cameraContorller.onResize(m_viewportSize.x, m_viewportSize.y);
	}

	// 拿到 FBO 里的纹理 ID
	textureID = m_framebuffer->getColorAttachmentRendererID();
	// 将 FBO 的内容绘制到 ImGui 窗口中
	// 注意：OpenGL 的 UV 坐标是 y 轴向上，ImGui 是向下，所以需要翻转 UV：{0, 1}, {1, 0}
	ImGui::Image((void*)(uintptr_t)textureID, ImVec2{ m_viewportSize.x, m_viewportSize.y }, ImVec2{ 0, 1 }, ImVec2{ 1, 0 });

	ImGui::End();
	ImGui::PopStyleVar();

	ImGui::End(); // 结束 DockSpace 宿主窗口
}

//void EditorLayer::onImGuiRender()
//{
//    // --- DockSpace 核心开始 ---
//    static bool dockspaceOpen = true;
//    static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;
//
//    // 设置主窗口标志（全屏、无标题、不可移动等）
//    ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
//    ImGuiViewport* viewport = ImGui::GetMainViewport();
//    ImGui::SetNextWindowPos(viewport->Pos);
//    ImGui::SetNextWindowSize(viewport->Size);
//    ImGui::SetNextWindowViewport(viewport->ID);
//    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
//    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
//    window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
//    window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
//
//    // 创建底层的 DockSpace 宿主窗口
//    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
//    ImGui::Begin("DockSpace Demo", &dockspaceOpen, window_flags);
//    ImGui::PopStyleVar();
//    ImGui::PopStyleVar(2); // 弹出之前 Push 的两个 StyleVar
//
//    // 激活 DockSpace
//    ImGuiIO& io = ImGui::GetIO();
//    if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
//    {
//        ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
//        ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
//    }
//
//    // 顶部菜单栏
//    if (ImGui::BeginMenuBar())
//    {
//        if (ImGui::BeginMenu("File"))
//        {
//            if (ImGui::MenuItem("Exit")) Rongine::Application::get().close();
//            ImGui::EndMenu();
//        }
//        ImGui::EndMenuBar();
//    }
//    // --- DockSpace 核心结束 ---
//
//    // ============================================================
//    //  DEMO: 类 3ds Max 样式工具栏 (Toolbar)
//    // ============================================================
//    // 1. 设置样式：让按钮紧凑，背景透明
//    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 2));
//    ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, ImVec2(0, 0));
//    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0)); // 默认按钮透明
//    auto& colors = ImGui::GetStyle().Colors;
//    const auto& buttonHovered = colors[ImGuiCol_ButtonHovered];
//    const auto& buttonActive = colors[ImGuiCol_ButtonActive];
//
//    // 2. 开启一个无标题栏的子窗口作为工具条
//    ImGui::Begin("##toolbar", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
//
//    // 准备图标纹理ID (这里暂时复用你的 Logo 和 棋盘格 作为演示图标)
//    // 实际项目中请加载专门的 icon (如: m_iconPlay, m_iconStop)
//    uint32_t iconTexID = m_logoTexture->getRendererID();
//    float size = 24.0f; // 图标大小
//
//    // --- 组 1: 撤销/重做 (使用文本按钮演示) ---
//    if (ImGui::Button("Undo")) { /* ... */ }
//    ImGui::SameLine();
//    if (ImGui::Button("Redo")) { /* ... */ }
//
//    ImGui::SameLine();
//    ImGui::TextDisabled("|"); // 简易分割线
//    ImGui::SameLine();
//
//    // --- 组 2: 变换工具 (互斥选择: 移动/旋转/缩放) ---
//    // 0: None, 1: Move, 2: Rotate, 3: Scale
//    static int m_gizmoType = 1;
//
//    // Lambda 辅助函数：绘制高亮 ImageButton
//    auto DrawToolButton = [&](int type, uint32_t texID) {
//        bool isActive = (m_gizmoType == type);
//        if (isActive) {
//            // 选中状态：深色背景高亮
//            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
//        }
//
//        // ImageButton (注意：ImGui ID 必须唯一，这里简单用 int 转换)
//        if (ImGui::ImageButton((ImTextureID)(uintptr_t)texID, ImVec2(size, size), ImVec2(0, 1), ImVec2(1, 0))) {
//            m_gizmoType = type;
//        }
//
//        if (isActive) {
//            ImGui::PopStyleColor();
//        }
//        ImGui::SameLine();
//        };
//
//    DrawToolButton(1, iconTexID); // 模拟 Move 图标
//    DrawToolButton(2, iconTexID); // 模拟 Rotate 图标
//    DrawToolButton(3, iconTexID); // 模拟 Scale 图标
//
//    ImGui::SameLine();
//    ImGui::TextDisabled("|");
//    ImGui::SameLine();
//
//    // --- 组 3: 捕捉开关 (Toggle) ---
//    static bool m_snapEnabled = false;
//
//    // 如果激活，把图标染成绿色，或者背景变亮
//    ImVec4 tintColor = m_snapEnabled ? ImVec4(0.6f, 1.0f, 0.6f, 1.0f) : ImVec4(1, 1, 1, 1);
//
//    // 这里暂时用 checkerboard 纹理区分
//    uint32_t snapIconID = m_checkerboardTexture->getRendererID();
//
//    if (ImGui::ImageButton((ImTextureID)(uintptr_t)snapIconID, ImVec2(size, size), ImVec2(0, 1), ImVec2(1, 0), -1, ImVec4(0, 0, 0, 0), tintColor))
//    {
//        m_snapEnabled = !m_snapEnabled;
//    }
//    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Snap Settings");
//
//    ImGui::End(); // 结束 Toolbar 窗口
//
//    // 恢复样式
//    ImGui::PopStyleVar(2);
//    ImGui::PopStyleColor();
//    // ============================================================
//
//
//    // 真正的 Settings 窗口（现在它可以被停靠到任何地方）
//    ImGui::Begin("Settings");
//
//    ImGui::ColorEdit4("Square Color", glm::value_ptr(m_squareColor));
//
//    // 显示当前选中的工具状态，验证 Toolbar 是否生效
//    ImGui::Separator();
//    ImGui::Text("Current Tool: %s", m_gizmoType == 1 ? "Move" : (m_gizmoType == 2 ? "Rotate" : "Scale"));
//    ImGui::Text("Snap Active: %s", m_snapEnabled ? "Yes" : "No");
//    ImGui::Separator();
//
//    auto stats = Rongine::Renderer2D::getStatistics();
//    ImGui::Text("Renderer2D Stats:");
//    ImGui::Text("Draw Calls: %d", stats.DrawCalls);
//    ImGui::Text("Quads: %d", stats.QuadCounts);
//    ImGui::Text("Vertices: %d", stats.getTotalVertexCounts());
//    ImGui::Text("Indices: %d", stats.getTotalIndexCounts());
//
//    // 渲染性能监控
//    ImGui::Separator();
//    for (auto& result : m_profileResult)
//    {
//        ImGui::Text("%.3fms  %s", result.time, result.name);
//    }
//    m_profileResult.clear();
//
//    // 模拟视口预览（将纹理渲染在窗口里）
//    uint32_t textureID = m_checkerboardTexture->getRendererID();
//    ImGui::Image((void*)(uintptr_t)textureID, ImVec2{ 256.0f, 256.0f }, { 0, 1 }, { 1, 0 });
//
//    ImGui::End(); // 结束 Settings 窗口
//
//    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 0, 0 }); // 让图片铺满窗口
//    ImGui::Begin("Viewport");
//
//    m_viewportFocused = ImGui::IsWindowFocused();
//    m_viewportHovered = ImGui::IsWindowHovered();
//    Rongine::Application::get().getImGuiLayer()->blockEvents(!m_viewportFocused && !m_viewportHovered);
//
//    // 拿到 FBO 里的纹理 ID
//    textureID = m_framebuffer->getColorAttachmentRendererID();
//
//    // 获取当前视口窗口可用的大小，而不是写死 1280x720
//    ImVec2 viewportSize = ImGui::GetContentRegionAvail();
//
//    // 将 FBO 的内容绘制到 ImGui 窗口中
//    ImGui::Image((void*)(uintptr_t)textureID, viewportSize, ImVec2{ 0, 1 }, ImVec2{ 1, 0 });
//
//    ImGui::End();
//    ImGui::PopStyleVar();
//
//    ImGui::End(); // 结束 DockSpace 宿主窗口
//}

void EditorLayer::onEvent(Rongine::Event& e)
{
	m_cameraContorller.onEvent(e);
}
