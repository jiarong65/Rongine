#include "Rongpch.h"
#include "EditorLayer.h"
#include "Platform/OpenGL/OpenGLShader.h"
#include "Rongine/Core/Input.h"
#include "Rongine/Core/KeyCodes.h"
#include "Rongine/Renderer/Renderer3D.h" // 必须包含这个
#include <glm/gtc/type_ptr.hpp>
#include <chrono>
#include <Bnd_Box.hxx>                
#include <BRepBndLib.hxx>             
#include <BRepPrimAPI_MakeSphere.hxx> 
#include <gp_Vec.hxx>                 
#include "imgui/imgui.h"

// --- Timer 类 (性能分析用) ---
template<typename Fn>
class Timer
{
public:
	Timer(const char* name, Fn&& func)
		:m_name(name), m_func(func), m_stopped(false)
	{
		m_startTimepoint = std::chrono::high_resolution_clock::now();
	}
	~Timer() { if (!m_stopped) stop(); }
	void stop() {
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

	Rongine::FramebufferSpecification fbSpec;
	fbSpec.width = 1280;
	fbSpec.height = 720;
	fbSpec.Attachments = {
		Rongine::FramebufferTextureFormat::RGBA8,
		Rongine::FramebufferTextureFormat::RED_INTEGER,
		Rongine::FramebufferTextureFormat::Depth
	};
	m_framebuffer = Rongine::Framebuffer::create(fbSpec);

	//// 【重要】初始化 Renderer3D
	//Rongine::Renderer3D::init();

	TopoDS_Shape shape = Rongine::CADImporter::ImportSTEP("assets/models/mypage.stp");
	m_CadMeshVA = Rongine::CADMesher::CreateMeshFromShape(shape);

	if(!m_CadMeshVA)RONG_CLIENT_ERROR("step 加载失败");

	m_TorusVA = Rongine::GeometryUtils::CreateTorus(1.0f, 0.4f, 64, 32);
}

void EditorLayer::onDetach()
{
	Rongine::Renderer3D::shutdown();
}

// 定义一个静态变量来控制旋转动画
static float s_Rotation = 0.0f;

void EditorLayer::onUpdate(Rongine::Timestep ts)
{
	PROFILE_SCOPE("EditorLayer::OnUpdate");

	// 1. 自动调整 Framebuffer 和相机大小
	if (m_viewportSize.x > 0.0f && m_viewportSize.y > 0.0f &&
		(m_framebuffer->getSpecification().width != m_viewportSize.x || m_framebuffer->getSpecification().height != m_viewportSize.y))
	{
		m_framebuffer->resize((uint32_t)m_viewportSize.x, (uint32_t)m_viewportSize.y);
		m_cameraContorller.onResize(m_viewportSize.x, m_viewportSize.y);
	}

	// 2. 相机输入控制
	if (m_viewportFocused)
		m_cameraContorller.onUpdate(ts);

	// 3. 更新旋转角度
	s_Rotation += ts * 45.0f; // 每秒转 45 度

	// 4. 开始渲染
	Rongine::Renderer3D::resetStatistics();
	{
		PROFILE_SCOPE("Renderer 3D");
		m_framebuffer->bind();
		Rongine::RenderCommand::setColor({ 0.1f, 0.1f, 0.1f, 1 });
		Rongine::RenderCommand::clear();

		m_framebuffer->clearAttachment(1, -1);

		Rongine::Renderer3D::beginScene(m_cameraContorller.getCamera());

		// A. 绘制地板 (静态)
		Rongine::Renderer3D::drawCube({ 0.0f, -1.0f, 0.0f }, { 100.0f, 0.1f, 100.0f }, m_checkerboardTexture);

		//// B. 绘制旋转方块阵列 (验证光照)
		//for (int x = -2; x <= 2; x++)
		//{
		//	for (int z = -2; z <= 2; z++)
		//	{
		//		glm::vec3 pos = { x * 1.5f, 0.0f, z * 1.5f };

		//		// 计算每个方块独特的旋转角度，产生波浪效果
		//		float angle = glm::radians(s_Rotation + (x + z) * 20.0f);

		//		// 颜色渐变
		//		glm::vec4 color = { (x + 2.0f) / 4.0f, 0.4f, (z + 2.0f) / 4.0f, 1.0f };

		//		// 调用旋转绘制函数
		//		Rongine::Renderer3D::drawRotatedCube(
		//			pos,
		//			{ 0.8f, 0.8f, 0.8f }, // 大小
		//			angle,
		//			{ 1.0f, 1.0f, 0.0f }, // 旋转轴 (对角线旋转，光照变化最明显)
		//			color
		//		);
		//	}
		//}

		//// C. 绘制带 Logo 的半透明旋转方块 (在中间上方)
		//Rongine::Renderer3D::drawRotatedCube(
		//	{ 0.0f, 2.0f, 0.0f },
		//	{ 1.5f, 1.5f, 1.5f },
		//	glm::radians(s_Rotation * 0.5f), // 转慢点
		//	{ 0.0f, 1.0f, 0.0f }, // 绕Y轴自转
		//	m_logoTexture
		//);

		if (m_TorusVA)
		{
			// 把它放在稍微高一点的位置，稍微转一下角度展示立体感
			glm::mat4 transform = glm::translate(glm::mat4(1.0f), { 0.0f, 1.5f, 0.0f })
				* glm::rotate(glm::mat4(1.0f), glm::radians(45.0f), { 1.0f, 0.0f, 0.0f });
			glDisable(GL_CULL_FACE);
			Rongine::Renderer3D::drawModel(m_TorusVA, transform);
			glEnable(GL_CULL_FACE);
		}

		if (m_CadMeshVA)
		{
			glm::mat4 transform = glm::translate(glm::mat4(1.0f), { 0.0f, 5.0f, 0.0f })
				* glm::scale(glm::mat4(1.0f), glm::vec3(0.03f));
			Rongine::Renderer3D::drawModel(m_CadMeshVA, transform);
		}

		Rongine::Renderer3D::endScene();

		m_framebuffer->unbind();
	}
}

void EditorLayer::onImGuiRender()
{
	// --- ImGui DockSpace 模板代码 ---
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
	ImGui::PopStyleVar(3); // Pop 3个样式变量

	ImGuiIO& io = ImGui::GetIO();
	if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
	{
		ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
		ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
	}

	if (ImGui::BeginMenuBar()) {
		if (ImGui::BeginMenu("File")) {
			if (ImGui::MenuItem("Exit")) Rongine::Application::get().close();
			ImGui::EndMenu();
		}
		ImGui::EndMenuBar();
	}

	// --- Settings 面板 ---
	ImGui::Begin("Settings");
	ImGui::ColorEdit4("Square Color", glm::value_ptr(m_squareColor));

	auto stats = Rongine::Renderer3D::getStatistics();
	ImGui::Text("Renderer3D Stats:");
	ImGui::Text("Draw Calls: %d", stats.DrawCalls);
	ImGui::Text("Cubes: %d", stats.CubeCount);
	ImGui::Text("Vertices: %d", stats.GetTotalVertexCount());

	ImGui::Separator();
	for (auto& result : m_profileResult)
		ImGui::Text("%.3fms  %s", result.time, result.name);
	m_profileResult.clear();
	ImGui::End();

	// --- Viewport 面板 ---
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 0, 0 });
	ImGui::Begin("Viewport");

	// 输入状态管理
	m_viewportFocused = ImGui::IsWindowFocused();
	m_viewportHovered = ImGui::IsWindowHovered();
	Rongine::Application::get().getImGuiLayer()->blockEvents(!m_viewportFocused && !m_viewportHovered);

	// 获取 Viewport 大小
	ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
	m_viewportSize = { viewportPanelSize.x, viewportPanelSize.y };

	// 绘制 FBO 纹理
	uint32_t textureID = m_framebuffer->getColorAttachmentRendererID();
	ImGui::Image((void*)(uintptr_t)textureID, ImVec2{ m_viewportSize.x, m_viewportSize.y }, ImVec2{ 0, 1 }, ImVec2{ 1, 0 });

	ImGui::End();
	ImGui::PopStyleVar();

	ImGui::End();
}

void EditorLayer::onEvent(Rongine::Event& e)
{
	m_cameraContorller.onEvent(e);
}