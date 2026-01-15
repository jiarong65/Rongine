#include "Rongpch.h"
#include "EditorLayer.h"
#include "Platform/OpenGL/OpenGLShader.h"
#include "Rongine/Core/Input.h"
#include "Rongine/Core/KeyCodes.h"
#include "Rongine/Renderer/Renderer3D.h" // 必须包含这个
#include "Rongine/Commands/DeleteCommand.h"
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
#include <TopoDS_Shape.hxx>
#include <BRepBuilderAPI_Transform.hxx>
#include <GProp_GProps.hxx>
#include <BRepGProp.hxx>
#include <BRepAlgoAPI_Fuse.hxx>
#include <BRepTools.hxx>

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
		Rongine::FramebufferTextureFormat::RGBA_INTEGER,
		Rongine::FramebufferTextureFormat::Depth
	};
	m_framebuffer = Rongine::Framebuffer::create(fbSpec);

	//////////////////////////////////////////////////////////////////////////

	m_activeScene = Rongine::CreateRef<Rongine::Scene>();

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
	//  开始渲染
	Rongine::Renderer3D::resetStatistics();
	{
		PROFILE_SCOPE("Renderer 3D");
		m_framebuffer->bind();
		Rongine::RenderCommand::setColor({ 0.1f, 0.1f, 0.1f, 1 });
		Rongine::RenderCommand::clear();

		m_framebuffer->clearAttachment(1, -1);

		//////////////////////////////////////////////////////////////////////////////////////////////////////////
		//批处理渲染
		Rongine::Renderer3D::beginScene(m_cameraContorller.getCamera());
		Rongine::Renderer3D::drawCube({ 0.0f, -1.0f, 0.0f }, { 100.0f, 0.1f, 100.0f }, m_checkerboardTexture);
		Rongine::Renderer3D::endScene();

		//////////////////////////////////////////////////////////////////////////////////////////////////////////
		//实例化渲染
		//传入选中的实体和面id
		int selectedEntityID = (int)(uint32_t)m_selectedEntity; // 这里的转换取决于你的 Entity 实现
		if (!m_selectedEntity) selectedEntityID = -2;

		Rongine::Renderer3D::setSelection(selectedEntityID, m_selectedFace);
		Rongine::Renderer3D::setHover(m_HoveredEntityID, m_HoveredFaceID, m_HoveredEdgeID);

		auto view = m_activeScene->getAllEntitiesWith<Rongine::TransformComponent, Rongine::MeshComponent>();

		for (auto entityHandle : view)
		{
			auto [transform, mesh] = view.get<Rongine::TransformComponent, Rongine::MeshComponent>(entityHandle);
			if (m_selectedEntity == entityHandle && mesh.EdgeVA)
			{
				glEnable(GL_POLYGON_OFFSET_FILL);

				// 设置偏移量：factor=1.0, units=1.0 是经验值
				// 这会让面的深度值增加，相当于往屏幕里推远了一点点
				glPolygonOffset(0.5f, 0.5f);
				Rongine::Renderer3D::drawModel(mesh.VA, transform.GetTransform(), (int)entityHandle);
				glDisable(GL_POLYGON_OFFSET_FILL);


				Rongine::Renderer3D::drawEdges(mesh.EdgeVA, transform.GetTransform(), { 0.0f, 0.0f, 0.0f, 1.0f }, (int)entityHandle, m_selectedEdge);
				continue;
			}
			Rongine::Renderer3D::drawModel(mesh.VA, transform.GetTransform(), (int)entityHandle);
		}
		////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//线框渲染
		Rongine::Renderer3D::beginLines(m_cameraContorller.getCamera());
		if (m_IsSketchMode && m_SketchPlaneEntity)
		{
			auto& sc = m_SketchPlaneEntity.GetComponent<Rongine::SketchComponent>();
			Rongine::Renderer3D::drawGrid(sc.SketchMatrix, 10.0f, 10);
		}
		Rongine::Renderer3D::endLines(); 
		////////////////////////////////////////////////////////////////////////////////////////////////////////////
		m_framebuffer->unbind();
	}

	//////////////////////////////////////////////////////////////////////////////
	//  鼠标拾取逻辑 (使用 m_viewportBounds)
	//////////////////////////////////////////////////////////////////////////////
	{
		// 1. 获取鼠标在视口内的坐标
		auto [mx, my] = ImGui::GetMousePos();
		mx -= m_viewportBounds[0].x;
		my -= m_viewportBounds[0].y;
		glm::vec2 viewportSize = m_viewportBounds[1] - m_viewportBounds[0];
		int mouseX = (int)mx;
		int mouseY = (int)(viewportSize.y - my);

		// 默认重置悬停状态 (假设没有悬停在任何东西上)
		m_HoveredEntityID = -1;
		m_HoveredFaceID = -1;
		m_HoveredEdgeID = -1;

		// ==============================================================================
		// Phase 1: 实时悬停探测 (Every Frame)
		// 只要鼠标在视口内，且没有在使用 Gizmo，就开始探测
		// ==============================================================================
		if (m_viewportHovered && !ImGuizmo::IsUsing() &&
			mouseX >= 0 && mouseY >= 0 && mouseX < (int)viewportSize.x && mouseY < (int)viewportSize.y)
		{
			m_framebuffer->bind();

			// 定义搜索半径 (悬停时也可以搜一点范围，提升手感)
			// 如果觉得卡顿，可以把这里的 radius 改为 0 或 1
			int radius = 2;

			int bestEntityID = -1;
			int bestFaceID = -1;
			int bestEdgeID = -1;
			float minDistanceSq = 10000.0f;

			// 遍历周围像素 (寻找最佳候选项)
			for (int dy = -radius; dy <= radius; dy++)
			{
				for (int dx = -radius; dx <= radius; dx++)
				{
					int x = mouseX + dx;
					int y = mouseY + dy;

					if (x < 0 || y < 0 || x >= (int)viewportSize.x || y >= (int)viewportSize.y)
						continue;

					// 读取像素 ID
					glm::ivec4 ids = m_framebuffer->readPixelID(1, x, y);
					int eID = ids.r;
					int fID = ids.g;
					int lID = ids.b; // EdgeID

					if (eID > -1)
					{
						float distSq = (float)(dx * dx + dy * dy);

						// --- 优先级策略 (边 > 面) ---
						bool isEdge = (lID > -1);
						float effectiveDist = distSq;

						// 如果是线，即使离得远一点，也视为距离为0 (强行优先)
						if (isEdge) effectiveDist -= 1000.0f;

						if (effectiveDist < minDistanceSq)
						{
							minDistanceSq = effectiveDist;
							bestEntityID = eID;
							bestFaceID = fID;
							bestEdgeID = lID;
						}
					}
				}
			}
			m_framebuffer->unbind();

			// 更新悬停状态变量
			if (bestEntityID > -1)
			{
				m_HoveredEntityID = bestEntityID;
				m_HoveredFaceID = bestFaceID;
				m_HoveredEdgeID = bestEdgeID;
			}
		}

		// ==============================================================================
		// Phase 2: 点击确认选择 (On Click)
		// 如果此时用户点击了左键，直接把刚才探测到的 Hover 结果应用为 Selected
		// ==============================================================================
		if (!m_IsExtrudeMode && m_viewportHovered && !ImGuizmo::IsUsing() && !ImGuizmo::IsOver() &&
			ImGui::IsMouseClicked(ImGuiMouseButton_Left))
		{
			if (m_HoveredEntityID > -1)
			{
				// 选中了物体
				m_selectedEntity = Rongine::Entity((entt::entity)m_HoveredEntityID, m_activeScene.get());
				m_sceneHierarchyPanel.setSelectedEntity(m_selectedEntity);

				// 优先选中边
				if (m_HoveredEdgeID > -1)
				{
					m_selectedEdge = m_HoveredEdgeID;
					m_selectedFace = -1;
					RONG_CLIENT_INFO("Picked Edge: {0}", m_selectedEdge);
				}
				// 其次选中面
				else if (m_HoveredFaceID > -1)
				{
					m_selectedFace = m_HoveredFaceID;
					m_selectedEdge = -1;
					RONG_CLIENT_INFO("Picked Face: {0}", m_selectedFace);
				}
				else
				{
					// 仅选中物体整体
					m_selectedFace = -1;
					m_selectedEdge = -1;
				}
			}
			else
			{
				// 点击了空白处：取消选择
				m_selectedEntity = {};
				m_selectedFace = -1;
				m_selectedEdge = -1;
			}

			// 同步 UI 状态
			m_sceneHierarchyPanel.setSelectedEdge(m_selectedEdge);
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

	/////////////////////////////////////////////////////////////////////////////////////////////
	// 1. 获取面板的选择
	auto selectedEntity = m_sceneHierarchyPanel.getSelectedEntity();

	// 2. 如果面板选择变了，同步给 Gizmo
	if (selectedEntity != m_selectedEntity)
		m_selectedEntity = selectedEntity;

	// ==================================================================

	///////////////////////////////////////////////////////////////////////
	if (ImGui::BeginMenuBar()) {
		if (ImGui::BeginMenu("File")) {
			if (ImGui::MenuItem("Import...", "Ctrl+I")) {
				ImportSTEP();
			}
			if (ImGui::MenuItem("Export Selected as STEP..."))
			{
				if (m_selectedEntity && m_selectedEntity.HasComponent<Rongine::CADGeometryComponent>())
				{
					std::string filepath = Rongine::FileDialogs::SaveFile("STEP File (*.step)\0*.step\0");
					if (!filepath.empty())
					{
						// 自动补全后缀
						if (filepath.find(".step") == std::string::npos && filepath.find(".stp") == std::string::npos)
							filepath += ".step";

						void* shape = m_selectedEntity.GetComponent<Rongine::CADGeometryComponent>().ShapeHandle;

						if (Rongine::CADExporter::ExportSTEP(filepath, shape))
						{
							RONG_CLIENT_INFO("Export Successful: {0}", filepath);
						}
					}
				}
				else
				{
					RONG_CLIENT_WARN("Please select a CAD object to export!");
				}
			}
			if (ImGui::MenuItem("Open...", "Ctrl+O")) {
				OpenScene(); // 改为调用 OpenScene
			}
			if (ImGui::MenuItem("Save As...", "Ctrl+S")) {
				SaveSceneAs(); // 新增 Save As
			}
			if (ImGui::MenuItem("Exit")) Rongine::Application::get().close();

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("GameObjects")) {
			if (ImGui::MenuItem("Create Cube"))
				CreatePrimitive(Rongine::CADGeometryComponent::GeometryType::Cube);
			if (ImGui::MenuItem("Create Sphere"))
				CreatePrimitive(Rongine::CADGeometryComponent::GeometryType::Sphere);
			if (ImGui::MenuItem("Create Cylinder"))
				CreatePrimitive(Rongine::CADGeometryComponent::GeometryType::Cylinder);

			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Edit"))
		{
			if (ImGui::MenuItem("Delete Selected", "Del"))
			{
				Rongine::CommandHistory::Push(new Rongine::DeleteCommand(m_selectedEntity));

				m_selectedEntity = {};
				m_sceneHierarchyPanel.setSelectedEntity({});
				m_selectedFace = -1;
				m_selectedEdge = -1;
				m_sceneHierarchyPanel.setSelectedEdge(-1);
			}
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Tools"))
		{
			// 设定工具物体
			if (ImGui::MenuItem("Set Selected as Tool (Knife)"))
			{
				if (m_selectedEntity)
				{
					m_ToolEntity = m_selectedEntity;
					RONG_CLIENT_INFO("Tool set to: {0}", (uint32_t)m_ToolEntity);
				}
			}

			ImGui::Separator();

			// 执行布尔运算 (当前选中 - 工具)
			if (ImGui::MenuItem("Boolean Cut (Selected - Tool)"))
			{
				OnBooleanOperation(Rongine::CADBoolean::Operation::Cut);
			}

			if (ImGui::MenuItem("Boolean Fuse (Selected + Tool)"))
			{
				OnBooleanOperation(Rongine::CADBoolean::Operation::Fuse);
			}

			if (ImGui::MenuItem("Boolean Common (Intersection)"))
			{
				OnBooleanOperation(Rongine::CADBoolean::Operation::Common);
			}
			if (ImGui::MenuItem("Interactive Extrude"))
			{
				EnterExtrudeMode();
			}
			if (ImGui::MenuItem("Interactive Fillet"))
			{
				int edgeID = m_selectedEdge;
				EnterFilletMode();
				ExitFilletMode(false);
				m_selectedEdge = edgeID;
				EnterFilletMode();
			}
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
		if(result.name=="EditorLayer::OnUpdate")ImGui::Text("FPS: %.3f", 1000.0f/result.time);
	m_profileResult.clear();
	ImGui::End();

	////////////////////////////////////////////////////////////////////////////////////////////
	// --- Viewport 面板 ---
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 0, 0 });
	ImGui::Begin("Viewport");

	// ===================== 工具栏区域 =====================

	ImGui::SetCursorPos(ImVec2(4, 4)); // 稍微向内偏移一点

	// 绘制按钮
	if (ImGui::Button("Box")) CreatePrimitive(Rongine::CADGeometryComponent::GeometryType::Cube);
	ImGui::SameLine();
	if (ImGui::Button("Sphere")) CreatePrimitive(Rongine::CADGeometryComponent::GeometryType::Sphere);
	ImGui::SameLine();
	if (ImGui::Button("Cylinder")) CreatePrimitive(Rongine::CADGeometryComponent::GeometryType::Cylinder);

	ImGui::SameLine();
	ImGui::Text("|");

	ImGui::SameLine();
	if (ImGui::Button("Extrude")) EnterExtrudeMode();
	ImGui::SameLine();
	if (ImGui::Button("Fillet")) EnterFilletMode();
	ImGui::SameLine();
	if (ImGui::Button("Sketch")) EnterSketchMode();

	// ============================================================

	// 计算工具栏的高度，以便把图片往下推
	// GetFrameHeightWithSpacing() 获取当前样式下一行的高度
	float toolbarHeight = ImGui::GetFrameHeightWithSpacing() + 4.0f; // +4 是上面的 SetCursorPos 偏移

	////////////////////////////////////////////////////////////////////////////////////////////
	// 计算视口边界 (需要考虑工具栏的高度偏移！)
	auto viewportMinRegion = ImGui::GetWindowContentRegionMin();
	auto viewportMaxRegion = ImGui::GetWindowContentRegionMax();
	auto viewportOffset = ImGui::GetWindowPos();

	// viewportBounds[0] 的 Y 轴需要加上工具栏高度，否则鼠标拾取会错位
	m_viewportBounds[0] = {
		viewportMinRegion.x + viewportOffset.x,
		viewportMinRegion.y + viewportOffset.y + toolbarHeight
	};
	m_viewportBounds[1] = {
		viewportMaxRegion.x + viewportOffset.x,
		viewportMaxRegion.y + viewportOffset.y
	};
	////////////////////////////////////////////////////////////////////////////////////////////

	// 输入状态管理
	m_viewportFocused = ImGui::IsWindowFocused();
	m_viewportHovered = ImGui::IsWindowHovered();
	Rongine::Application::get().getImGuiLayer()->blockEvents(!m_viewportFocused && !m_viewportHovered);

	// 获取 Viewport 大小 (减去工具栏高度)
	ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();

	ImGui::SetCursorPos(ImVec2(0, toolbarHeight));

	// 更新视口大小变量 (减去工具栏高度)
	m_viewportSize = { viewportPanelSize.x, viewportPanelSize.y - toolbarHeight };

	// 防止最小化时出现负数导致崩溃
	if (m_viewportSize.x < 0) m_viewportSize.x = 1;
	if (m_viewportSize.y < 0) m_viewportSize.y = 1;

	////////////////////////////////////////////////////////////////////////////////////////////
	// 绘制 FBO 纹理
	uint32_t textureID = m_framebuffer->getColorAttachmentRendererID();
	// 使用计算后的大小
	ImGui::Image((void*)(uintptr_t)textureID, ImVec2{ m_viewportSize.x, m_viewportSize.y }, ImVec2{ 0, 1 }, ImVec2{ 1, 0 });

	/////////////////////////////////////////////////////////////////////////////////////////////
	// ImGuizmo
	// 1. 设置上下文
	ImGuizmo::SetOrthographic(false);
	ImGuizmo::SetDrawlist();
	ImGuizmo::SetRect(m_viewportBounds[0].x, m_viewportBounds[0].y,
		m_viewportBounds[1].x - m_viewportBounds[0].x,
		m_viewportBounds[1].y - m_viewportBounds[0].y);

	// ================== 拉伸模式 ==================
	if (m_IsExtrudeMode)
	{
		// 1. 获取相机矩阵
		const auto& camera = m_cameraContorller.getCamera();
		glm::mat4 cameraView = camera.getViewMatrix();
		const glm::mat4& cameraProjection = camera.getProjectionMatrix();

		// 2. 准备 Gizmo 矩阵
		// 我们需要把 Gizmo 放在 m_ExtrudeGizmoMatrix 的位置
		// 并且我们把当前的拉伸量 m_ExtrudeHeight 叠加到 Z 轴上显示

		glm::mat4 currentGizmoMatrix = glm::translate(m_ExtrudeGizmoMatrix, glm::vec3(0, 0, m_ExtrudeHeight));

		// 3. 绘制 Gizmo (只显示 Z 轴平移)
		// 使用 ImGuizmo::TRANSLATE_Z 可以限制只沿 Z 轴移动
		// 注意：ImGuizmo 的 Z 轴对应我们的法线方向
		ImGuizmo::Manipulate(glm::value_ptr(cameraView), glm::value_ptr(cameraProjection),
			ImGuizmo::TRANSLATE_Z, ImGuizmo::LOCAL, glm::value_ptr(currentGizmoMatrix));

		// 4. 处理拖拽
		if (ImGuizmo::IsUsing())
		{
			// 计算相对于初始位置的偏移量
			// currentGizmoMatrix 的第 4 列 (Translation) 与 m_ExtrudeGizmoMatrix 的差值
			// 或者更简单：我们只需要知道这次操作移动了多少

			// 为了简单，我们反解 Z 轴的移动量
			// (这里简化处理：假设只操作了 Translate Z)

			// 更稳健的方法：计算 currentGizmoMatrix 原点 到 m_ExtrudeGizmoMatrix 原点的距离，并投影到 Z 轴
			glm::vec3 startPos = m_ExtrudeGizmoMatrix[3];
			glm::vec3 currentPos = currentGizmoMatrix[3];
			glm::vec3 normal = m_ExtrudeGizmoMatrix[2]; // Z轴方向

			// 投影向量 (Current - Start) 到 Normal 上
			float newHeight = glm::dot(currentPos - startPos, glm::normalize(normal));

			// 如果高度变了，更新预览
			if (std::abs(newHeight - m_ExtrudeHeight) > 0.01f) // 加个阈值减少计算频率
			{
				m_ExtrudeHeight = newHeight;

				// 生成预览网格
				auto& cadComp = m_selectedEntity.GetComponent<Rongine::CADGeometryComponent>();
				void* previewShape = Rongine::CADFeature::ExtrudeFace(cadComp.ShapeHandle, m_selectedFace, m_ExtrudeHeight);

				if (previewShape)
				{
					// 同样要变换到世界坐标系
					// 注意：ExtrudeFace 生成的是局部形状，我们需要给 PreviewEntity 设置正确的 Parent Transform
					auto& prevTC = m_PreviewEntity.GetComponent<Rongine::TransformComponent>();
					auto& parentTC = m_selectedEntity.GetComponent<Rongine::TransformComponent>();
					prevTC.Translation = parentTC.Translation;
					prevTC.Rotation = parentTC.Rotation;
					prevTC.Scale = parentTC.Scale;

					// 更新 Mesh
					std::vector<Rongine::CubeVertex> verts;
					auto va = Rongine::CADMesher::CreateMeshFromShape(*(TopoDS_Shape*)previewShape, verts, 0.5f); // 预览精度低一点没关系

					if (m_PreviewEntity.HasComponent<Rongine::MeshComponent>())
						m_PreviewEntity.RemoveComponent<Rongine::MeshComponent>(); // 移除旧的

					m_PreviewEntity.AddComponent<Rongine::MeshComponent>(va, verts);

					// 记得清理 previewShape 指针，因为它只是临时的
					delete (TopoDS_Shape*)previewShape;
				}
			}
		}

		// 屏幕 UI 按钮
		// 在视口左上角画两个按钮，方便用户退出
		// 设置光标位置到视口内部上方
		ImGui::SetCursorPos(ImVec2(20, 40));

		// 绿色确认按钮
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0.8f, 0, 1));
		if (ImGui::Button("Apply (Enter)"))
		{
			ExitExtrudeMode(true);
		}
		ImGui::PopStyleColor();

		ImGui::SameLine();

		// 红色取消按钮
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0, 0, 1));
		if (ImGui::Button("Cancel (Esc)"))
		{
			ExitExtrudeMode(false);
		}
		ImGui::PopStyleColor();

		// 显示当前高度提示
		ImGui::SameLine();
		ImGui::Text("Height: %.2f", m_ExtrudeHeight);
		// =============================================================

		// 5. 键盘退出 (保留这个作为快捷键)
		if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Enter)) || Rongine::Input::isKeyPressed(Rongine::Key::Enter))
		{
			ExitExtrudeMode(true); // 确认
		}
		if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Escape)) || Rongine::Input::isKeyPressed(Rongine::Key::Escape))
		{
			ExitExtrudeMode(false); // 取消
		}

		// 既然在拉伸模式，就不要运行下面的普通 Gizmo 逻辑了
		goto SkipGizmo;
	}
	///////////////////////////////////////////////////////////////////////////////////////////
	//倒角模式
	if (m_IsFilletMode)
	{
		const auto& camera = m_cameraContorller.getCamera();
		glm::mat4 cameraView = camera.getViewMatrix();
		const glm::mat4& cameraProjection = camera.getProjectionMatrix();

		// 1. 准备 Gizmo
		// 我们根据 m_FilletRadius 偏移 Gizmo，给用户视觉反馈
		// 假设沿 X 轴拖拽调整半径
		glm::mat4 currentGizmoMatrix = glm::translate(m_FilletGizmoMatrix, glm::vec3(m_FilletRadius, 0, 0));

		// 2. 绘制 Gizmo (只显示 X 轴平移，代表半径大小)
		ImGuizmo::Manipulate(glm::value_ptr(cameraView), glm::value_ptr(cameraProjection),
			ImGuizmo::TRANSLATE_X, ImGuizmo::LOCAL, glm::value_ptr(currentGizmoMatrix));

		// 3. 处理拖拽
		if (ImGuizmo::IsUsing())
		{
			// 计算位移量作为半径
			glm::vec3 startPos = m_FilletGizmoMatrix[3];
			glm::vec3 currentPos = currentGizmoMatrix[3];

			// 简单地取距离或者 X 轴投影
			float newRadius = glm::length(currentPos - startPos);
			// 或者： float newRadius = currentPos.x - startPos.x; (如果局部坐标系对齐好了)

			// 限制半径范围
			if (newRadius < 0.01f) newRadius = 0.01f;

			if (std::abs(newRadius - m_FilletRadius) > 0.01f)
			{
				m_FilletRadius = newRadius;

				// ==================== 生成预览网格 ====================
				// 注意：这里我们不修改原物体，而是生成一个临时的 TopoDS_Shape
				auto& cadComp = m_selectedEntity.GetComponent<Rongine::CADGeometryComponent>();

				// 需要一个静态函数：MakeFilletShape(originalShape, edgeID, radius) -> newShape*
				void* previewShape = Rongine::CADFeature::MakeFilletShape(cadComp.ShapeHandle, m_selectedEdge, m_FilletRadius);

				if (previewShape)
				{
					// 更新预览实体的 Mesh
					std::vector<Rongine::CubeVertex> verts;
					auto va = Rongine::CADMesher::CreateMeshFromShape(*(TopoDS_Shape*)previewShape, verts, 0.5f); // 预览精度

					if (m_PreviewEntity.HasComponent<Rongine::MeshComponent>())
						m_PreviewEntity.RemoveComponent<Rongine::MeshComponent>();

					m_PreviewEntity.AddComponent<Rongine::MeshComponent>(va, verts);

					delete (TopoDS_Shape*)previewShape; // 只是预览，用完即弃
				}
			}
		}

		// 4. UI 按钮 (确认/取消)
		ImGui::SetCursorPos(ImVec2(20, 40));
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0.8f, 0, 1));
		if (ImGui::Button("Apply Fillet (Enter)")) ExitFilletMode(true);
		ImGui::PopStyleColor();

		ImGui::SameLine();
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0, 0, 1));
		if (ImGui::Button("Cancel (Esc)")) ExitFilletMode(false);
		ImGui::PopStyleColor();

		ImGui::SameLine();
		ImGui::Text("Radius: %.2f", m_FilletRadius);

		// 5. 快捷键
		if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Enter))) ExitFilletMode(true);
		if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Escape))) ExitFilletMode(false);

		goto SkipGizmo; // 跳过普通 Gizmo
	}

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

		bool isParametricCAD = false;
		if (m_selectedEntity.HasComponent<Rongine::CADGeometryComponent>())
		{
			auto type = m_selectedEntity.GetComponent<Rongine::CADGeometryComponent>().Type;
			// 只有 Cube, Sphere, Cylinder 是参数化的，Imported (导入模型) 依然允许缩放
			if (type != Rongine::CADGeometryComponent::GeometryType::Imported &&
				type != Rongine::CADGeometryComponent::GeometryType::None)
			{
				isParametricCAD = true;
			}
		}
		// 如果是参数化模型，且当前处于缩放模式 (R键)，则强制不显示 Gizmo，也不允许操作
		if (isParametricCAD && m_gizmoType == ImGuizmo::SCALE)
		{
			// 你可以选择什么都不做，或者在屏幕上画个提示
			// 这里的 continue/return 取决于你的代码结构，如果是单独的函数直接 return
			// 简单做法：把 gizmoType 临时设为 -1 (隐藏)，或者直接跳过下面的 Manipulate 调用
			// 但为了不破坏状态，最简单的办法是：什么都不画，直接跳过本帧 Gizmo 逻辑

			// 跳出 Gizmo 逻辑块 (假设这是在一个大 if 里，或者你可以把下面的逻辑包在一个 else 里)
			goto SkipGizmo;
		}

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
			// A. 刚开始拖拽的那一帧：记录起点
			if (!m_GizmoEditing)
			{
				m_GizmoEditing = true;
				m_GizmoStartTransform = m_selectedEntity.GetComponent<Rongine::TransformComponent>();
			}

			glm::vec3 translation, rotation, scale;
			glm::vec3 skew;
			glm::vec4 perspective;
			glm::quat orientation;

			glm::decompose(gizmoMatrix, scale, orientation, translation, skew, perspective);

			auto& tc = m_selectedEntity.GetComponent<Rongine::TransformComponent>();

			// --- 实时更新 (为了让用户看到拖拽效果) ---
			// 注意：这里我们依然直接修改组件，否则用户拖不动！
			// 撤销系统的作用是在松手那一刻，把“旧状态 -> 新状态”记录下来。

			if (m_gizmoType == ImGuizmo::SCALE)
			{
				// 针对 CAD 模型的缩放拦截逻辑 (保持你之前的 SkipGizmo)
				tc.Scale = scale;
			}
			else
			{
				if (snapToFace)
				{
					glm::mat4 rotMat = glm::toMat4(orientation);
					glm::mat4 scaleMat = glm::scale(glm::mat4(1.0f), scale);
					glm::mat4 rsMat = rotMat * scaleMat;
					glm::vec3 newOffset = glm::vec3(rsMat * glm::vec4(faceLocalCenter, 1.0f));
					tc.Translation = translation - newOffset;
				}
				else
				{
					tc.Translation = translation;
				}
				tc.Rotation = glm::eulerAngles(orientation);
			}
		}
		else
		{
			// B. 松开鼠标的那一帧：提交命令
			if (m_GizmoEditing)
			{
				m_GizmoEditing = false;

				// 获取拖拽结束后的状态
				auto endTransform = m_selectedEntity.GetComponent<Rongine::TransformComponent>();

				// 只有当真的移动了才产生命令
				if (m_GizmoStartTransform.Translation != endTransform.Translation ||
					m_GizmoStartTransform.Rotation != endTransform.Rotation ||
					m_GizmoStartTransform.Scale != endTransform.Scale)
				{
					// 创建并压入命令
					// 注意：Execute() 会再次设置一次 Transform，但这没关系，因为数据是一样的
					Rongine::CommandHistory::Push(new Rongine::TransformCommand(m_selectedEntity, m_GizmoStartTransform, endTransform));

					RONG_CLIENT_INFO("Gizmo Transform Command Pushed");
				}
			}
		}
	}

SkipGizmo:;


	ImGui::End(); // End Viewport (Viewport 结束)
	ImGui::PopStyleVar();

	ImGui::End(); // End Dockspace (Dockspace 结束)
}

void EditorLayer::onEvent(Rongine::Event& e)
{
	m_cameraContorller.onEvent(e);

	//快捷键
	Rongine::EventDispatcher dispatcher(e);
	dispatcher.dispatch<Rongine::KeyPressedEvent>([this](Rongine::KeyPressedEvent& event) -> bool
	{
		// 检查 Ctrl 是否按下
		bool control = Rongine::Input::isKeyPressed(Rongine::Key::LeftControl) ||
			Rongine::Input::isKeyPressed(Rongine::Key::RightControl);
		bool shift = Rongine::Input::isKeyPressed(Rongine::Key::LeftShift) || 
			Rongine::Input::isKeyPressed(Rongine::Key::RightShift);

		if (control)
		{
			if (event.getKeyCode() == Rongine::Key::Z)
			{
				Rongine::CommandHistory::Undo();
				return true; // 吞掉事件
			}
			if (event.getKeyCode() == Rongine::Key::Y)
			{
				Rongine::CommandHistory::Redo();
				return true;
			}
			if (event.getKeyCode() ==Rongine::Key::I)
			{
				ImportSTEP();
				return true;
			}
			if (event.getKeyCode() == Rongine::Key::O)
			{
				OpenScene();
				return true;
			}
			if (event.getKeyCode() == Rongine::Key::S)
			{
				SaveSceneAs();
				return true;
			}
		}

		if (event.getKeyCode() == Rongine::Key::Delete)
		{
			if (m_selectedEntity)
			{
				Rongine::CommandHistory::Push(new Rongine::DeleteCommand(m_selectedEntity));

				m_selectedEntity = {};
				m_sceneHierarchyPanel.setSelectedEntity({});
				m_selectedFace = -1;
				m_selectedEdge = -1;
				m_sceneHierarchyPanel.setSelectedEdge(-1);
			}
			return true;
		}

		//  自动对焦 (Frame Selection)
		if (event.getKeyCode() == Rongine::Key::F)
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
				return true;
			}
		}

		return false;
	});
}

void EditorLayer::ImportSTEP()
{
	std::string filepath = Rongine::FileDialogs::OpenFile("STEP Files (*.step;*.stp)\0*.step;*.stp\0All Files (*.*)\0*.*\0");

	if (!filepath.empty())
	{
		m_selectedEntity = {};

		TopoDS_Shape shape = Rongine::CADImporter::ImportSTEP(filepath);

		std::vector<Rongine::CubeVertex> verticesData;
		auto cadMeshVA = Rongine::CADMesher::CreateMeshFromShape(shape,verticesData);

		if (cadMeshVA) {
			auto cadEntity = m_activeScene->createEntity("Imported CAD");
			auto& tc = cadEntity.GetComponent<Rongine::TransformComponent>();

			auto& meshComp = cadEntity.AddComponent<Rongine::MeshComponent>(cadMeshVA, verticesData);

			// ==================== 生成边框线 ====================
			std::vector<Rongine::LineVertex> lineVerts;
			auto edgeVA = Rongine::CADMesher::CreateEdgeMeshFromShape(shape, lineVerts, meshComp.m_IDToEdgeMap, 0.1f);
			meshComp.EdgeVA = edgeVA;
			meshComp.LocalLines = lineVerts;
			// ===========================================================

			meshComp.BoundingBox = Rongine::CADImporter::CalculateAABB(shape);
			RONG_CLIENT_INFO("Successfully imported: {0}", filepath);
		}
		else {
			RONG_CLIENT_ERROR("Failed to load: {0}", filepath);
		}
	}
}

void EditorLayer::CreatePrimitive(Rongine::CADGeometryComponent::GeometryType type)
{
	// 1. 创建 Entity
	auto entity = m_activeScene->createEntity("New Object");

	// 2. 添加基础组件
	entity.AddComponent<Rongine::TransformComponent>();

	// 3. 添加 CAD 组件并生成数据
	auto& cadComp = entity.AddComponent<Rongine::CADGeometryComponent>();
	cadComp.Type = type;

	void* shapeHandle = nullptr;

	if (type == Rongine::CADGeometryComponent::GeometryType::Cube)
	{
		cadComp.Params.Width = 1.0f;
		cadComp.Params.Height = 1.0f;
		cadComp.Params.Depth = 1.0f;
		shapeHandle = Rongine::CADModeler::MakeCube(1.0f, 1.0f, 1.0f);
	}
	else if (type == Rongine::CADGeometryComponent::GeometryType::Sphere)
	{
		cadComp.Params.Radius = 1.0f;
		shapeHandle = Rongine::CADModeler::MakeSphere(1.0f);
	}
	else if (type == Rongine::CADGeometryComponent::GeometryType::Cylinder)
	{
		cadComp.Params.Radius = 1.0f;
		cadComp.Params.Height = 2.0f;
		shapeHandle = Rongine::CADModeler::MakeCylinder(1.0f, 2.0f);
	}

	cadComp.ShapeHandle = shapeHandle;

	// 4. 关键一步：立即离散化生成 Mesh (用于渲染)
	if (shapeHandle)
	{
		TopoDS_Shape* occShape = (TopoDS_Shape*)shapeHandle;

		std::vector<Rongine::CubeVertex> verticesData;
		// 调用你之前修改好的 CreateMeshFromShape
		auto va = Rongine::CADMesher::CreateMeshFromShape(*occShape, verticesData);

		if (va)
		{
			// 创建 Mesh 组件，并存入 CPU 顶点数据 (这对 Gizmo 吸附很重要！)
			auto& meshComp=entity.AddComponent<Rongine::MeshComponent>(va, verticesData);

			std::vector<Rongine::LineVertex> lineVerts;
			// 默认精度 0.1f
			auto edgeVA = Rongine::CADMesher::CreateEdgeMeshFromShape(*occShape, lineVerts, meshComp.m_IDToEdgeMap, 0.1f);

			meshComp.EdgeVA = edgeVA;
			meshComp.LocalLines = lineVerts;

			// 计算包围盒
			meshComp.BoundingBox = Rongine::CADImporter::CalculateAABB(*occShape);

			RONG_CLIENT_INFO("Created Primitive Successfully!");
		}
	}
}

void EditorLayer::SaveSceneAs()
{
	// 打开保存文件对话框，过滤 .rong 文件
	std::string filepath = Rongine::FileDialogs::SaveFile("Rongine Scene (*.rong)\0*.rong\0");

	if (!filepath.empty())
	{
		// 创建序列化器并保存当前场景
		Rongine::SceneSerializer serializer(m_activeScene);
		serializer.Serialize(filepath);

		RONG_CLIENT_INFO("Scene saved to: {0}", filepath);
	}
}

void EditorLayer::OpenScene()
{
	// 打开文件对话框
	std::string filepath = Rongine::FileDialogs::OpenFile("Rongine Scene (*.rong)\0*.rong\0");

	if (!filepath.empty())
	{
		// 1. 创建一个新的空场景（把旧的扔掉）
		m_activeScene = Rongine::CreateRef<Rongine::Scene>();

		// 2. 如果视口大小已知，调整新场景视口（防止画面拉伸）
		//if (m_viewportSize.x > 0 && m_viewportSize.y > 0)
		//	m_activeScene->onViewportResize((uint32_t)m_viewportSize.x, (uint32_t)m_viewportSize.y);

		// 3. 更新面板上下文（让 Hierarchy 面板显示新场景）
		m_sceneHierarchyPanel.setContext(m_activeScene);
		m_selectedEntity = {}; // 清空之前的选择，防止野指针
		m_selectedFace = -1;

		// 4. 反序列化（核心步骤！）
		Rongine::SceneSerializer serializer(m_activeScene);
		if (serializer.Deserialize(filepath))
		{
			RONG_CLIENT_INFO("Scene loaded from: {0}", filepath);
		}
		else
		{
			RONG_CLIENT_ERROR("Failed to load scene: {0}", filepath);
		}
	}
}

void EditorLayer::OnBooleanOperation(Rongine::CADBoolean::Operation op)
{
	// 检查有效性：必须选中了物体，且工具物体也存在，且两者不是同一个
	if (!m_selectedEntity || !m_ToolEntity || m_selectedEntity == m_ToolEntity)
	{
		RONG_CLIENT_WARN("Invalid Boolean Operands! Select one entity and set another as Tool.");
		return;
	}

	// 检查是否都有 CAD 组件
	if (!m_selectedEntity.HasComponent<Rongine::CADGeometryComponent>() ||
		!m_ToolEntity.HasComponent<Rongine::CADGeometryComponent>())
	{
		RONG_CLIENT_WARN("Both entities must have CAD Geometry!");
		return;
	}

	auto& cadA = m_selectedEntity.GetComponent<Rongine::CADGeometryComponent>();
	auto& transA = m_selectedEntity.GetComponent<Rongine::TransformComponent>();

	auto& cadB = m_ToolEntity.GetComponent<Rongine::CADGeometryComponent>();
	auto& transB = m_ToolEntity.GetComponent<Rongine::TransformComponent>();

	// 1. 执行运算
	void* resultShapeHandle = Rongine::CADBoolean::Perform(
		cadA.ShapeHandle, transA.GetTransform(),
		cadB.ShapeHandle, transB.GetTransform(),
		op
	);

	// 2. 如果成功，生成新物体
	if (resultShapeHandle)
	{

		TopoDS_Shape* occShape = (TopoDS_Shape*)resultShapeHandle;

		// ==================== 【新增】几何中心归位逻辑 ====================

		// A. 计算几何中心
		GProp_GProps props;
		BRepGProp::VolumeProperties(*occShape, props);
		gp_Pnt center = props.CentreOfMass(); // 获取质心

		// B. 创建一个反向变换：把形状从中心移回原点
		gp_Trsf transform;
		transform.SetTranslation(center, gp_Pnt(0, 0, 0));

		// C. 应用变换到形状 (永久修改形状的顶点)
		BRepBuilderAPI_Transform xform(*occShape, transform);
		TopoDS_Shape centeredShape = xform.Shape();

		// D. 更新 shapeHandle 指向这个归位后的形状
		// (注意：这里要小心内存管理，最好把旧的 occShape 删掉，或者直接用 new)
		*occShape = centeredShape; // 简单粗暴覆盖

		// =================================================================
		// 创建结果实体
		auto resultEntity = m_activeScene->createEntity("Boolean Result");

		// 添加 Transform
		// 注意：因为 Perform 已经在世界坐标系下运算了，所以新物体的 Transform 应该是归零的
		auto& resTc = resultEntity.AddComponent<Rongine::TransformComponent>();
		resTc.Translation = { (float)center.X(), (float)center.Y(), (float)center.Z() };

		// 添加 CAD 组件 (类型设为 Imported，因为它已经不是纯参数化的球/方块了)
		auto& resCad = resultEntity.AddComponent<Rongine::CADGeometryComponent>();
		resCad.Type = Rongine::CADGeometryComponent::GeometryType::Imported;
		resCad.ShapeHandle = resultShapeHandle;

		// 生成 Mesh
		std::vector<Rongine::CubeVertex> vertices;
		auto va = Rongine::CADMesher::CreateMeshFromShape(*occShape, vertices);

		if (va)
		{
			auto& meshComp = resultEntity.AddComponent<Rongine::MeshComponent>(va, vertices);

			// ==================== 生成边框线 ====================
			std::vector<Rongine::LineVertex> lineVerts;
			// 这里如果有 cadComp，最好用 cadComp.LinearDeflection，这里简化用 0.1f 或继承
			auto edgeVA = Rongine::CADMesher::CreateEdgeMeshFromShape(*occShape, lineVerts, meshComp.m_IDToEdgeMap, 0.1f);
			meshComp.EdgeVA = edgeVA;
			meshComp.LocalLines = lineVerts;
			// ===========================================================
			meshComp.BoundingBox = Rongine::CADImporter::CalculateAABB(*occShape);

			if (m_selectedEntity.HasComponent<Rongine::TagComponent>())
				m_selectedEntity.GetComponent<Rongine::TagComponent>().Tag += " (Hidden)";
			if (m_ToolEntity.HasComponent<Rongine::TagComponent>())
				m_ToolEntity.GetComponent<Rongine::TagComponent>().Tag += " (Hidden)";

			m_selectedEntity = resultEntity;
			m_sceneHierarchyPanel.setSelectedEntity(resultEntity);
		}

		// 可选：运算后隐藏或删除原物体
		// m_activeScene->DestroyEntity(m_selectedEntity);
		// m_activeScene->DestroyEntity(m_ToolEntity);

		RONG_CLIENT_INFO("Boolean Operation Successful!");
	}
}

void EditorLayer::EnterExtrudeMode()
{
	if (!m_selectedEntity || m_selectedFace == -1) return;
	if (!m_selectedEntity.HasComponent<Rongine::CADGeometryComponent>()) return;

	m_IsExtrudeMode = true;
	m_ExtrudeHeight = 0.0f;

	// 1. 获取选中面的坐标系，作为 Gizmo 的初始位置
	// 注意：这个矩阵是局部空间的还是世界空间的？
	// GetFaceTransform 返回的是局部坐标系下的点。
	// 我们需要把它转到世界坐标系，以便 ImGuizmo 正确显示。

	auto& tc = m_selectedEntity.GetComponent<Rongine::TransformComponent>();
	auto& cadComp = m_selectedEntity.GetComponent<Rongine::CADGeometryComponent>();

	glm::mat4 faceLocalMatrix = Rongine::CADFeature::GetFaceTransform(cadComp.ShapeHandle, m_selectedFace);
	glm::mat4 objectWorldMatrix = tc.GetTransform();

	// 组合：Gizmo 世界矩阵 = 物体变换 * 面局部变换
	m_ExtrudeGizmoMatrix = objectWorldMatrix * faceLocalMatrix;

	// 2. 创建一个空的预览实体 (Ghost)
	m_PreviewEntity = m_activeScene->createEntity("Extrude Preview");
	auto& prevTC = m_PreviewEntity.HasComponent<Rongine::TransformComponent>() ?
		m_PreviewEntity.GetComponent<Rongine::TransformComponent>() :
		m_PreviewEntity.AddComponent<Rongine::TransformComponent>();
	// 给个半透明材质或者特殊颜色更好 (这里暂时用默认 Mesh)

	RONG_CLIENT_INFO("Entered Extrude Mode. Drag the Blue Arrow!");
}

void EditorLayer::ExitExtrudeMode(bool apply)
{
	if (!m_IsExtrudeMode) return;

	if (apply && std::abs(m_ExtrudeHeight) > 0.001f)
	{
		auto& cadComp = m_selectedEntity.GetComponent<Rongine::CADGeometryComponent>();

		auto* cmd = new Rongine::CADModifyCommand(m_selectedEntity);

		void* prismPtr = Rongine::CADFeature::ExtrudeFace(cadComp.ShapeHandle, m_selectedFace, m_ExtrudeHeight);

		if (prismPtr)
		{
			TopoDS_Shape* baseShape = (TopoDS_Shape*)cadComp.ShapeHandle;
			TopoDS_Shape* toolShape = (TopoDS_Shape*)prismPtr;

			// 2. 布尔运算 (融合)
			BRepAlgoAPI_Fuse fuseAlgo(*baseShape, *toolShape);
			fuseAlgo.Build();

			if (fuseAlgo.IsDone())
			{
				// 3. 更新组件数据 
				if (cadComp.ShapeHandle) delete (TopoDS_Shape*)cadComp.ShapeHandle;

				// 应用新 Shape
				cadComp.ShapeHandle = new TopoDS_Shape(fuseAlgo.Shape());
				cadComp.Type = Rongine::CADGeometryComponent::GeometryType::Imported;

				// 4. 重建网格
				Rongine::CADMesher::RebuildMesh(m_selectedEntity);

				cmd->CaptureNewState();
				Rongine::CommandHistory::Push(cmd);

				RONG_CLIENT_INFO("Extrude Applied.");
			}
			else
			{
				delete cmd; // 运算失败，撤销该命令
			}
			delete (TopoDS_Shape*)prismPtr; // 清理临时拉伸体
		}
		else
		{
			delete cmd; // 没生成拉伸体，清理命令
		}
	}

	if (m_PreviewEntity)
	{
		m_activeScene->destroyEntity(m_PreviewEntity);
		m_PreviewEntity = {};
	}

	m_IsExtrudeMode = false;
	m_ExtrudeHeight = 0.0f;

	m_selectedFace = -1;
}

void EditorLayer::EnterFilletMode()
{
	Rongine::Entity id = m_selectedEntity;
	// 1. 检查选中有效性 (必须选中一条边)
	if (!m_selectedEntity || m_selectedEdge == -1)
	{
		RONG_CLIENT_WARN("Please select an Edge first!");
		return;
	}
	if (!m_selectedEntity.HasComponent<Rongine::CADGeometryComponent>()) return;

	m_IsFilletMode = true;
	m_FilletRadius = 0.2f; // 初始半径
	m_FilletEdge = m_selectedEdge;

	// 2. 计算 Gizmo 位置 (放在选中边的中点)
	auto& tc = m_selectedEntity.GetComponent<Rongine::TransformComponent>();
	auto& cadComp = m_selectedEntity.GetComponent<Rongine::CADGeometryComponent>();

	glm::mat4 edgeLocalMatrix = Rongine::CADFeature::GetEdgeTransform(cadComp.ShapeHandle, m_selectedEdge);
	glm::mat4 objectWorldMatrix = tc.GetTransform();

	// Gizmo 位于边的中心
	m_FilletGizmoMatrix = objectWorldMatrix * edgeLocalMatrix;

	// 3. 创建预览实体 (Ghost)
	m_PreviewEntity = m_activeScene->createEntity("Fillet Preview");

	Rongine::TransformComponent& previewTC = m_PreviewEntity.GetComponent<Rongine::TransformComponent>();

	// 复制 Transform
	previewTC.Translation = tc.Translation;
	previewTC.Rotation = tc.Rotation;
	previewTC.Scale = tc.Scale;


	// 复制当前的 Mesh (作为初始状态)
	if (m_selectedEntity.HasComponent<Rongine::MeshComponent>())
	{
		auto& srcMesh = m_selectedEntity.GetComponent<Rongine::MeshComponent>();

		//复制srcMesh，因为操作AddComponent后，内存池可能会满，会重新申请一片内存，释放原来的内存扩容
		auto srcVA = srcMesh.VA;
		auto srcVertices = srcMesh.LocalVertices;
		auto srcEdgeVA = srcMesh.EdgeVA;
		auto srcLines = srcMesh.LocalLines;
		auto srcBoundingBox = srcMesh.BoundingBox;
		auto srcIDMap = srcMesh.m_IDToEdgeMap;

		if (m_PreviewEntity.HasComponent<Rongine::MeshComponent>())
			m_PreviewEntity.RemoveComponent<Rongine::MeshComponent>();

		// 复制 VA 和顶点数据
		auto& dstMesh = m_PreviewEntity.AddComponent<Rongine::MeshComponent>(srcMesh.VA, srcMesh.LocalVertices);


		dstMesh.EdgeVA = srcEdgeVA;
		dstMesh.LocalLines = srcLines;
		dstMesh.BoundingBox = srcBoundingBox;
	}

	// 强制触发一次预览更新
	// 因为设置了初始半径 0.2
	{
		void* previewShape = Rongine::CADFeature::MakeFilletShape(cadComp.ShapeHandle, m_selectedEdge, m_FilletRadius);
		RONG_CLIENT_INFO("MakeFilletShape");
		if (previewShape)
		{
			RONG_CLIENT_INFO("MakeFilletShape su");
			// 更新预览实体的 Mesh
			std::vector<Rongine::CubeVertex> verts;
			auto va = Rongine::CADMesher::CreateMeshFromShape(*(TopoDS_Shape*)previewShape, verts, 0.5f); // 预览精度

			if (m_PreviewEntity.HasComponent<Rongine::MeshComponent>())
				m_PreviewEntity.RemoveComponent<Rongine::MeshComponent>();

			m_PreviewEntity.AddComponent<Rongine::MeshComponent>(va, verts);

			delete (TopoDS_Shape*)previewShape; // 只是预览，用完即弃
		}
	}
	m_selectedEntity = id;

	RONG_CLIENT_INFO("Entered Fillet Mode. Drag the Gizmo to adjust radius!");
}

void EditorLayer::ExitFilletMode(bool apply)
{
	if (!m_IsFilletMode) return;

	if (apply && m_FilletRadius > 0.001f)
	{
		auto& cadComp = m_selectedEntity.GetComponent<Rongine::CADGeometryComponent>();

		// 1. 创建 Undo 命令 (备份当前状态)
		auto* cmd = new Rongine::CADModifyCommand(m_selectedEntity);
		// B. 执行操作
		Rongine::CADMesher::ApplyFillet(m_selectedEntity,m_FilletEdge,m_FilletRadius);

		// C. 备份新状态并提交
		cmd->CaptureNewState();
		Rongine::CommandHistory::Push(cmd);

		// D. 强制取消选择 (防止拓扑变动导致崩溃)
		m_selectedEdge = -1;
	}

	// 清理预览实体
	if (m_PreviewEntity)
	{
		m_activeScene->destroyEntity(m_PreviewEntity);
		m_PreviewEntity = {};
	}

	// 重置状态
	m_IsFilletMode = false;
	m_FilletRadius = 0.0f;
	m_FilletEdge = -1;
	m_selectedEdge = -1; // 倒角后 Edge ID 会变，必须重置选择
}

// EditorLayer.cpp

void EditorLayer::EnterSketchMode()
{
	if (!m_selectedEntity || m_selectedFace == -1)
	{
		RONG_CLIENT_WARN("Please select a Planar Face to start Sketching!");
		return;
	}

	// 检查 CAD 组件
	if (!m_selectedEntity.HasComponent<Rongine::CADGeometryComponent>()) return;
	auto& cad = m_selectedEntity.GetComponent<Rongine::CADGeometryComponent>();

	// 计算面坐标系
	gp_Ax3 sketchAx3;
	glm::mat4 sketchMat;

	if (Rongine::CADFeature::GetPlanarFaceCoordinateSystem(*(TopoDS_Shape*)cad.ShapeHandle, m_selectedFace, sketchAx3, sketchMat))
	{
		m_IsSketchMode = true;
		m_SketchPlaneEntity = m_selectedEntity;

		auto& sc = m_SketchPlaneEntity.HasComponent<Rongine::SketchComponent>() ?
			m_SketchPlaneEntity.GetComponent<Rongine::SketchComponent>() :
			m_SketchPlaneEntity.AddComponent<Rongine::SketchComponent>();

		sc.IsActive = true;
		sc.PlaneLocalSystem = sketchAx3;
		sc.SketchMatrix = sketchMat;

		glm::vec3 center = glm::vec3(sketchMat[3]); // 草图原点 (Target)
		glm::vec3 normal = glm::vec3(sketchMat[2]); // 草图法线 (Z轴)

		// 基于 AABB 包围盒
		float viewDistance = 5.0f; // 默认备用距离

		if (m_selectedEntity.HasComponent<Rongine::MeshComponent>())
		{
			auto& mesh = m_selectedEntity.GetComponent<Rongine::MeshComponent>();

			// 获取包围盒尺寸
			glm::vec3 size = mesh.BoundingBox.GetSize();

			// 取长宽高中的最大值作为参考维度
			float maxDim = std::max({ size.x, size.y, size.z });

			// 根据 FOV 计算能容纳物体的距离
			// 公式: distance = (size / 2) / tan(fov / 2)
			float fovRad = glm::radians(m_cameraContorller.getFOV());

			// 防止除以 0 
			if (fovRad > 0.0f)
			{
				viewDistance = (maxDim * 1.0f) / std::tan(fovRad / 2.0f);
			}

			// 限制最小距离
			viewDistance = std::max(viewDistance, 1.0f);
		}

		glm::vec3 camPos = center + normal * viewDistance;
		m_cameraContorller.lookAt(camPos, center);

		RONG_CLIENT_INFO("Entered Sketch Mode! Camera aligned to face.");
	}
	else
	{
		RONG_CLIENT_ERROR("Selected face is not planar! Cannot sketch on curved surfaces yet.");
	}
}
