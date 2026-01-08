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
#include "Rongine/Scene/Components.h"
#include "ImGuizmo.h" 
#include <glm/gtx/matrix_decompose.hpp>

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
		Rongine::FramebufferTextureFormat::RG_INTEGER,
		Rongine::FramebufferTextureFormat::Depth
	};
	m_framebuffer = Rongine::Framebuffer::create(fbSpec);

	//////////////////////////////////////////////////////////////////////////

	m_activeScene = Rongine::CreateRef<Rongine::Scene>();

	// 2. 加载 CAD 实体
	TopoDS_Shape shape = Rongine::CADImporter::ImportSTEP("assets/models/mypage.stp");

	std::vector<Rongine::CubeVertex> verticesData;
	auto cadMeshVA = Rongine::CADMesher::CreateMeshFromShape(shape,verticesData); // 使用局部变量

	if (cadMeshVA) {
		auto cadEntity = m_activeScene->createEntity("CAD Model");
		auto& tc = cadEntity.GetComponent<Rongine::TransformComponent>();
		tc.Translation = { 0.0f, 5.0f, 0.0f };
		tc.Scale = { 0.03f, 0.03f, 0.03f };
		cadEntity.AddComponent<Rongine::MeshComponent>(cadMeshVA, verticesData);
		cadEntity.GetComponent<Rongine::MeshComponent>().BoundingBox = Rongine::CADImporter::CalculateAABB(shape);
	}
	else {
		RONG_CLIENT_ERROR("step 加载失败");
	}

	// 3. 创建 Torus 实体
	auto torusVA = Rongine::GeometryUtils::CreateTorus(1.0f, 0.4f, 64, 32);
	if (torusVA) {
		auto torusEntity = m_activeScene->createEntity("Torus");
		auto& tc = torusEntity.GetComponent<Rongine::TransformComponent>();
		tc.Translation = { 0.0f, 1.5f, 0.0f };
		tc.Rotation = { glm::radians(45.0f), 0.0f, 0.0f };
		torusEntity.AddComponent<Rongine::MeshComponent>(torusVA);
		auto& box = torusEntity.GetComponent<Rongine::MeshComponent>().BoundingBox;
		box.Max = { -1.4f, -0.4f, -1.4f };
		box.Min = { 1.4f, 0.4f, 1.4f };
	}

	//////////////////////////////////////////////////////////////////////////
	//ui面板
	m_sceneHierarchyPanel.setContext(m_activeScene);
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

	//////////////////////////////////////////////////////////////////////////////////////////
	if (Rongine::Input::isKeyPressed(Rongine::Key::Q)) m_gizmoType = -1;
	if (Rongine::Input::isKeyPressed(Rongine::Key::W)) m_gizmoType = ImGuizmo::TRANSLATE;
	if (Rongine::Input::isKeyPressed(Rongine::Key::E)) m_gizmoType = ImGuizmo::ROTATE;
	if (Rongine::Input::isKeyPressed(Rongine::Key::R)) m_gizmoType = ImGuizmo::SCALE;
	//////////////////////////////////////////////////////////////////////////////////////////
	// 快捷键检测
	bool control = Rongine::Input::isKeyPressed(Rongine::Key::LeftControl) || Rongine::Input::isKeyPressed(Rongine::Key::RightControl);
	bool shift = Rongine::Input::isKeyPressed(Rongine::Key::LeftShift) || Rongine::Input::isKeyPressed(Rongine::Key::RightShift);

	if (control && Rongine::Input::isKeyPressed(Rongine::Key::O))
	{
		OpenFile();
	}

	//  自动对焦 (Frame Selection)
	if (Rongine::Input::isKeyPressed(Rongine::Key::F))
	{
		if (m_selectedEntity && m_selectedEntity.HasComponent<Rongine::MeshComponent>())
		{
			// 1. 获取组件
			auto& transform = m_selectedEntity.GetComponent<Rongine::TransformComponent>();
			auto& mesh = m_selectedEntity.GetComponent<Rongine::MeshComponent>();

			// 2. 获取原始 AABB
			Rongine::AABB aabb = mesh.BoundingBox;

			// 3. 计算变换后的 AABB 中心点 (作为新的焦点)
			// 注意：这里做了一个简单的假设，物体中心 = 变换后的 AABB 中心
			// 更严谨的做法是变换 AABB 的 8 个顶点再求新的 min/max
			glm::mat4 modelMatrix = transform.GetTransform();
			glm::vec3 center = glm::vec3(modelMatrix * glm::vec4(aabb.GetCenter(), 1.0f));

			// 计算合适的相机距离
			// 获取物体最大尺寸 (应用缩放)
			glm::vec3 size = aabb.GetSize() * transform.Scale;
			float radius = glm::length(size) * 0.5f;

			// 简单三角学计算：distance = radius / sin(FOV/2)
			// 2.0f 是一个宽松系数，让物体不要填得太满，留点边距
			float fovRad = glm::radians(45.0f); // 假设相机 FOV 是 45 度
			float distance = radius / std::sin(fovRad / 2.0f);

			m_cameraContorller.setFocus(center, distance);

			RONG_CLIENT_INFO("Focused on Entity ID: {0}", (uint32_t)m_selectedEntity);
		}
	}
	//////////////////////////////////////////////////////////////////////////////////////////
	//  开始渲染
	Rongine::Renderer3D::resetStatistics();
	{
		PROFILE_SCOPE("Renderer 3D");
		m_framebuffer->bind();
		Rongine::RenderCommand::setColor({ 0.1f, 0.1f, 0.1f, 1 });
		Rongine::RenderCommand::clear();

		m_framebuffer->clearAttachment(1, -1);

		Rongine::Renderer3D::beginScene(m_cameraContorller.getCamera());

		//传入选中的实体和面id
		int selectedEntityID = (int)(uint32_t)m_selectedEntity; // 这里的转换取决于你的 Entity 实现
		if (!m_selectedEntity) selectedEntityID = -2;

		Rongine::Renderer3D::setSelection(selectedEntityID, m_selectedFace);

		// A. 绘制地板 (静态)
		Rongine::Renderer3D::drawCube({ 0.0f, -1.0f, 0.0f }, { 100.0f, 0.1f, 100.0f }, m_checkerboardTexture);

		// 获取所有拥有 Transform 和 Mesh 的实体
		auto view = m_activeScene->getAllEntitiesWith<Rongine::TransformComponent, Rongine::MeshComponent>();

		for (auto entityHandle : view)
		{
			auto [transform, mesh] = view.get<Rongine::TransformComponent, Rongine::MeshComponent>(entityHandle);
			// 将 entityHandle 强转为 int 作为 ID 传入 shader
			Rongine::Renderer3D::drawModel(mesh.VA, transform.GetTransform(), (int)entityHandle);
		}

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

		//if (m_TorusVA)
		//{
		//	// 把它放在稍微高一点的位置，稍微转一下角度展示立体感
		//	glm::mat4 transform = glm::translate(glm::mat4(1.0f), { 0.0f, 1.5f, 0.0f })
		//		* glm::rotate(glm::mat4(1.0f), glm::radians(45.0f), { 1.0f, 0.0f, 0.0f });
		//	Rongine::Renderer3D::drawModel(m_TorusVA, transform,1);
		//}

		//if (m_CadMeshVA)
		//{
		//	glm::mat4 transform = glm::translate(glm::mat4(1.0f), { 0.0f, 5.0f, 0.0f })
		//		* glm::scale(glm::mat4(1.0f), glm::vec3(0.03f));
		//	Rongine::Renderer3D::drawModel(m_CadMeshVA, transform,2);
		//}

		Rongine::Renderer3D::endScene();

		m_framebuffer->unbind();
	}

	//////////////////////////////////////////////////////////////////////////////
	//  鼠标拾取逻辑 (使用 m_viewportBounds)
	//////////////////////////////////////////////////////////////////////////////
	{
		if (m_viewportHovered && !ImGuizmo::IsUsing() && !ImGuizmo::IsOver() &&
			ImGui::IsMouseClicked(ImGuiMouseButton_Left))
		{
			auto [mx, my] = ImGui::GetMousePos();
			mx -= m_viewportBounds[0].x;
			my -= m_viewportBounds[0].y;
			glm::vec2 viewportSize = m_viewportBounds[1] - m_viewportBounds[0];
			int mouseX = (int)mx;
			int mouseY = (int)(viewportSize.y - my);

			if (mouseX >= 0 && mouseY >= 0 && mouseX < (int)viewportSize.x && mouseY < (int)viewportSize.y)
			{
				m_framebuffer->bind();

				std::pair<int, int> pixelData = m_framebuffer->readPixelRG(1, mouseX, mouseY);
				int entityID = pixelData.first;
				int faceID = pixelData.second;
				//int pixelID = m_framebuffer->readPixel(1, mouseX, mouseY);
				m_framebuffer->unbind();

				// [核心修改 2] 只有当确实点到了东西才更新
				// 如果点到了 -1 (背景)，这里处理为取消选择
				if (entityID > -1)
				{
					m_selectedEntity = Rongine::Entity((entt::entity)entityID, m_activeScene.get());
					m_selectedFace = faceID;
					m_sceneHierarchyPanel.setSelectedEntity(m_selectedEntity);

					RONG_CLIENT_INFO("Picked Entity: {0}, Face: {1}", entityID, faceID);
				}
				else
				{
					// 点击空白处取消选择
					m_selectedEntity = {};
					m_selectedFace = -1;
				}
			}
		}
	}
}

void EditorLayer::onImGuiRender()
{

	ImGuizmo::BeginFrame();
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
			if (ImGui::MenuItem("Open...", "Ctrl+O")) {
				OpenFile();
			}

			if (ImGui::MenuItem("Exit")) Rongine::Application::get().close();
			ImGui::EndMenu();
		}
		ImGui::EndMenuBar();
	}

	////////////////////////////////////////////////////////////////////////////////////////////
	m_sceneHierarchyPanel.onImGuiRender();

	////////////////////////////////////////////////////////////////////////////////////////////
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

	////////////////////////////////////////////////////////////////////////////////////////////
	// --- Viewport 面板 ---
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 0, 0 });
	ImGui::Begin("Viewport");

	////////////////////////////////////////////////////////////////////////////////////////////
	// 计算视口边界并存入 m_viewportBounds
	auto viewportMinRegion = ImGui::GetWindowContentRegionMin();
	auto viewportMaxRegion = ImGui::GetWindowContentRegionMax();
	auto viewportOffset = ImGui::GetWindowPos();

	// 计算绝对屏幕坐标
	m_viewportBounds[0] = { viewportMinRegion.x + viewportOffset.x, viewportMinRegion.y + viewportOffset.y };
	m_viewportBounds[1] = { viewportMaxRegion.x + viewportOffset.x, viewportMaxRegion.y + viewportOffset.y };
	////////////////////////////////////////////////////////////////////////////////////////////

	// 输入状态管理
	m_viewportFocused = ImGui::IsWindowFocused();
	m_viewportHovered = ImGui::IsWindowHovered();
	Rongine::Application::get().getImGuiLayer()->blockEvents(!m_viewportFocused && !m_viewportHovered);

	// 获取 Viewport 大小
	ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
	m_viewportSize = { viewportPanelSize.x, viewportPanelSize.y };

	////////////////////////////////////////////////////////////////////////////////////////////
	// 绘制 FBO 纹理
	uint32_t textureID = m_framebuffer->getColorAttachmentRendererID();
	ImGui::Image((void*)(uintptr_t)textureID, ImVec2{ m_viewportSize.x, m_viewportSize.y }, ImVec2{ 0, 1 }, ImVec2{ 1, 0 });

	/////////////////////////////////////////////////////////////////////////////////////////////
	// ImGuizmo
	// 1. 设置上下文
	ImGuizmo::SetOrthographic(false);
	ImGuizmo::SetDrawlist();
	ImGuizmo::SetRect(m_viewportBounds[0].x, m_viewportBounds[0].y,
		m_viewportBounds[1].x - m_viewportBounds[0].x,
		m_viewportBounds[1].y - m_viewportBounds[0].y);

	// 2. 绘制逻辑
	if (m_selectedEntity && m_gizmoType != -1)
	{

		// 获取相机的 View 和 Projection 矩阵
		const auto& camera = m_cameraContorller.getCamera();
		const glm::mat4& cameraProjection = camera.getProjectionMatrix();
		glm::mat4 cameraView = camera.getViewMatrix();
		// 1. 获取基础数据
		auto& tc = m_selectedEntity.GetComponent<Rongine::TransformComponent>();
		glm::mat4 objectTransform = tc.GetTransform();

		// 2. 准备 Gizmo 显示用的矩阵
		// 默认情况：Gizmo 在物体原点
		glm::mat4 gizmoMatrix = objectTransform;

		// 标记是否处于“面吸附”模式
		bool snapToFace = false;
		glm::vec3 faceLocalCenter(0.0f);

		// --- 吸附逻辑 ---
		// 如果选中了面，且该实体有网格数据
		if (m_selectedFace > -1 && m_selectedEntity.HasComponent<Rongine::MeshComponent>())
		{
			auto& mesh = m_selectedEntity.GetComponent<Rongine::MeshComponent>();
			// 确保我们在 Step 1 中存的 LocalVertices 不为空
			if (!mesh.LocalVertices.empty())
			{
				// A. 算出局部中心
				auto faceInfo = Rongine::GeometryUtils::CalculateFaceCenter(mesh.LocalVertices, m_selectedFace);
				faceLocalCenter = faceInfo.LocalCenter;

				// B. 将局部中心转为世界坐标 (应用物体的旋转和缩放)
				// 公式：WorldPos = ObjectMatrix * LocalPos
				glm::vec4 worldCenter4 = objectTransform * glm::vec4(faceInfo.LocalCenter, 1.0f);
				glm::vec3 worldCenter = glm::vec3(worldCenter4);

				// C. 强制修改 Gizmo 矩阵的位置列 (第4列)，让 Gizmo 跳到面中心
				// 注意：我们只改位置，不改旋转和缩放，所以 Gizmo 看起来方向还是正的
				gizmoMatrix[3][0] = worldCenter.x;
				gizmoMatrix[3][1] = worldCenter.y;
				gizmoMatrix[3][2] = worldCenter.z;

				snapToFace = true;
			}
		}

		// 3. 绘制 Gizmo (注意：操作的是 gizmoMatrix)
		// 这样用户看到 Gizmo 就在面的中心
		ImGuizmo::Manipulate(glm::value_ptr(cameraView), glm::value_ptr(cameraProjection),
			(ImGuizmo::OPERATION)m_gizmoType, ImGuizmo::LOCAL, glm::value_ptr(gizmoMatrix));

		// 4. 处理用户输入 (反向应用回物体)
		if (ImGuizmo::IsUsing())
		{
			glm::vec3 translation, rotation, scale;
			glm::vec3 skew;
			glm::vec4 perspective;
			glm::quat orientation;

			// 分解 被用户移动后的 Gizmo 矩阵
			glm::decompose(gizmoMatrix, scale, orientation, translation, skew, perspective);

			// --- 核心数学推导 ---
			if (snapToFace)
			{
				// 如果 Gizmo 在面中心，我们需要算回物体的原点在哪里
				// 物体原点 = Gizmo新位置 - (新旋转缩放后的面偏移)

				// 1. 重建旋转缩放矩阵
				glm::mat4 rotMat = glm::toMat4(orientation);
				glm::mat4 scaleMat = glm::scale(glm::mat4(1.0f), scale);
				glm::mat4 rsMat = rotMat * scaleMat;

				// 2. 计算此时面中心相对于原点的偏移量
				glm::vec3 newOffset = glm::vec3(rsMat * glm::vec4(faceLocalCenter, 1.0f));

				// 3. 应用回物体组件
				tc.Translation = translation - newOffset;
			}
			else
			{
				// 普通模式：Gizmo 位置即物体位置
				tc.Translation = translation;
			}

			// 旋转和缩放直接应用
			tc.Rotation = glm::eulerAngles(orientation);
			tc.Scale = scale;
		}
	}

	/////////////////////////////////////////////////////////////////////////////////////////////
	// 1. 获取面板的选择
	auto selectedEntity = m_sceneHierarchyPanel.getSelectedEntity();

	// 2. 如果面板选择变了，同步给 Gizmo
	if (selectedEntity != m_selectedEntity)
		m_selectedEntity = selectedEntity;

	// ==================================================================

	ImGui::End(); // End Viewport (Viewport 结束)
	ImGui::PopStyleVar();

	ImGui::End(); // End Dockspace (Dockspace 结束)
}

void EditorLayer::onEvent(Rongine::Event& e)
{
	m_cameraContorller.onEvent(e);
}

void EditorLayer::OpenFile()
{
	std::string filepath = Rongine::FileDialogs::OpenFile("STEP Files (*.step;*.stp)\0*.step;*.stp\0All Files (*.*)\0*.*\0");

	if (!filepath.empty())
	{
		m_activeScene = Rongine::CreateRef<Rongine::Scene>();
		//m_activeScene->onViewportResize((uint32_t)m_viewportSize.x, (uint32_t)m_viewportSize.y);

		m_sceneHierarchyPanel.setContext(m_activeScene);
		m_selectedEntity = {};

		TopoDS_Shape shape = Rongine::CADImporter::ImportSTEP(filepath);

		std::vector<Rongine::CubeVertex> verticesData;
		auto cadMeshVA = Rongine::CADMesher::CreateMeshFromShape(shape,verticesData);

		if (cadMeshVA) {
			auto cadEntity = m_activeScene->createEntity("Imported CAD");
			auto& tc = cadEntity.GetComponent<Rongine::TransformComponent>();

			cadEntity.AddComponent<Rongine::MeshComponent>(cadMeshVA,verticesData);
			cadEntity.GetComponent<Rongine::MeshComponent>().BoundingBox = Rongine::CADImporter::CalculateAABB(shape);
			RONG_CLIENT_INFO("Successfully imported: {0}", filepath);
		}
		else {
			RONG_CLIENT_ERROR("Failed to load: {0}", filepath);
		}
	}
}
