#include "Rongpch.h"
#include "SceneHierarchyPanel.h"
#include "Rongine/Scene/Components.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "Rongine/CAD/CADModeler.h"
#include "Rongine/CAD/CADMesher.h"
#include "Rongine/CAD/CADImporter.h"
#include <glm/gtc/type_ptr.hpp>
#include <BRepTools.hxx>

namespace Rongine {

	static void BeginDisabled(bool disabled = true)
	{
		if (disabled)
		{
			// 变灰：通过降低透明度实现
			ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
			// 禁用交互：使用内部标志
			ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
		}
	}

	static void EndDisabled(bool disabled = true)
	{
		if (disabled)
		{
			ImGui::PopItemFlag();
			ImGui::PopStyleVar();
		}
	}

	static void RebuildCADGeometry(Entity entity)
	{
		if (!entity.HasComponent<CADGeometryComponent>() || !entity.HasComponent<MeshComponent>())
			return;

		auto& cadComp = entity.GetComponent<CADGeometryComponent>();
		auto& meshComp = entity.GetComponent<MeshComponent>();

		const float MIN_SIZE = 0.001f;
		// 1. 清理旧的 Shape (防止内存泄漏)
		if (cadComp.ShapeHandle)
		{
			CADModeler::FreeShape(cadComp.ShapeHandle);
			cadComp.ShapeHandle = nullptr;
		}

		// 2. 根据当前参数生成新的 Shape
		void* newShapeHandle = nullptr;
		if (cadComp.Type == CADGeometryComponent::GeometryType::Cube)
		{
			float w = std::max(cadComp.Params.Width, MIN_SIZE);
			float h = std::max(cadComp.Params.Height, MIN_SIZE);
			float d = std::max(cadComp.Params.Depth, MIN_SIZE);

			// 把修正后的值写回组件，这样 UI 上也会变成 0.001 而不是停留在 0
			cadComp.Params.Width = w;
			cadComp.Params.Height = h;
			cadComp.Params.Depth = d;

			newShapeHandle = CADModeler::MakeCube(cadComp.Params.Width, cadComp.Params.Height, cadComp.Params.Depth);
		}
		else if (cadComp.Type == CADGeometryComponent::GeometryType::Sphere)
		{
			float r = std::max(cadComp.Params.Radius, MIN_SIZE);
			cadComp.Params.Radius = r;

			newShapeHandle = CADModeler::MakeSphere(cadComp.Params.Radius);
		}
		else if (cadComp.Type == CADGeometryComponent::GeometryType::Cylinder)
		{
			float r = std::max(cadComp.Params.Radius, MIN_SIZE);
			float h = std::max(cadComp.Params.Height, MIN_SIZE);

			cadComp.Params.Radius = r;
			cadComp.Params.Height = h;

			newShapeHandle = CADModeler::MakeCylinder(cadComp.Params.Radius, cadComp.Params.Height);
		}

		cadComp.ShapeHandle = newShapeHandle;

		// 3. 重新生成 Mesh (离散化)
		if (newShapeHandle)
		{
			TopoDS_Shape* occShape = (TopoDS_Shape*)newShapeHandle;
			BRepTools::Clean(*occShape);
			std::vector<CubeVertex> newVertices;

			// 调用 Mesher 生成新数据
			auto newVA = CADMesher::CreateMeshFromShape(*occShape, newVertices,cadComp.LinearDeflection);

			if (newVA)
			{
				// 4. 更新 MeshComponent
				meshComp.VA = newVA;
				meshComp.LocalVertices = newVertices; // 更新 CPU 顶点，保证 Gizmo 吸附正确
				meshComp.BoundingBox = CADImporter::CalculateAABB(*occShape); // 更新包围盒，保证 F 键聚焦正确

				// ==================== 生成边框线 ====================
				std::vector<LineVertex> lineVerts;
				auto edgeVA = CADMesher::CreateEdgeMeshFromShape(*occShape, lineVerts, cadComp.LinearDeflection);
				meshComp.EdgeVA = edgeVA;
				meshComp.LocalLines = lineVerts;
				// ===========================================================
			}
		}
	}

	// 修改精度，重新生成网格 (不重新创建数学模型)
	static void RebuildMeshOnly(Entity entity)
	{
		if (!entity.HasComponent<CADGeometryComponent>() || !entity.HasComponent<MeshComponent>())
			return;

		auto& cadComp = entity.GetComponent<CADGeometryComponent>();
		auto& meshComp = entity.GetComponent<MeshComponent>();

		if (cadComp.ShapeHandle)
		{
			TopoDS_Shape* occShape = (TopoDS_Shape*)cadComp.ShapeHandle;
			BRepTools::Clean(*occShape);
			std::vector<CubeVertex> newVertices;

			// 传入当前的 精度
			auto newVA = CADMesher::CreateMeshFromShape(*occShape, newVertices, cadComp.LinearDeflection);

			if (newVA)
			{
				meshComp.VA = newVA;
				meshComp.LocalVertices = newVertices;
				// 更新AABB
				meshComp.BoundingBox = CADImporter::CalculateAABB(*occShape);

				// ==================== 生成边框线 ====================
				std::vector<LineVertex> lineVerts;
				// 传入 cadComp.LinearDeflection
				meshComp.m_IDToEdgeMap.clear();
				auto edgeVA = CADMesher::CreateEdgeMeshFromShape(
					*occShape,
					lineVerts,
					meshComp.m_IDToEdgeMap, 
					cadComp.LinearDeflection
				);
				meshComp.EdgeVA = edgeVA;
				meshComp.LocalLines = lineVerts;
				// ===========================================================
			}
		}
	}

	SceneHierarchyPanel::SceneHierarchyPanel(const Ref<Scene>& context)
	{
		setContext(context);
	}

	void SceneHierarchyPanel::setContext(const Ref<Scene>& context)
	{
		m_context = context;
		m_selectionContext = {}; // 切换场景时清空选择
	}

	void SceneHierarchyPanel::setSelectedEntity(Entity entity)
	{
		m_selectionContext = entity;
	}

	void SceneHierarchyPanel::onImGuiRender()
	{
		// 1. 场景层级面板 (Hierarchy)
		ImGui::Begin("Scene Hierarchy");

		if (m_context)
		{
			// 遍历所有实体
			// 这里使用 EnTT 的 view 遍历所有 TagComponent (基本上所有实体都有 Tag)
			auto view =m_context->getRegistry().view<TagComponent>();
			for (auto entityID : view)
			{
				Entity entity{ entityID , m_context.get() };
				drawEntityNode(entity);
			}

			// 点击空白处取消选择
			if (ImGui::IsMouseDown(0) && ImGui::IsWindowHovered())
				m_selectionContext = {};
		}
		ImGui::End();

		// 2. 属性面板 (Properties)
		ImGui::Begin("Properties");
		if (m_selectionContext)
		{
			drawComponents(m_selectionContext);
		}
		ImGui::End();
	}

	void SceneHierarchyPanel::drawEntityNode(Entity entity)
	{
		auto& tag = entity.GetComponent<TagComponent>().Tag;

		ImGuiTreeNodeFlags flags = ((m_selectionContext == entity) ? ImGuiTreeNodeFlags_Selected : 0) | ImGuiTreeNodeFlags_OpenOnArrow;

		// 绘制树节点，使用实体 ID 作为唯一标识符
		bool opened = ImGui::TreeNodeEx((void*)(uint64_t)(uint32_t)entity, flags, tag.c_str());

		if (ImGui::IsItemClicked())
		{
			m_selectionContext = entity;
		}

		// 暂时没有子节点，如果有 RelationshipComponent，这里要递归
		if (opened)
		{
			ImGui::TreePop();
		}
	}

	void SceneHierarchyPanel::drawComponents(Entity entity)
	{
		// --- Tag (名字) ---
		if (entity.HasComponent<TagComponent>())
		{
			auto& tag = entity.GetComponent<TagComponent>().Tag;

			char buffer[256];
			memset(buffer, 0, sizeof(buffer));
			strcpy_s(buffer, sizeof(buffer), tag.c_str());

			// 输入框，允许改名
			if (ImGui::InputText("Tag", buffer, sizeof(buffer)))
			{
				tag = std::string(buffer);
			}
		}

		// --- Transform (变换) ---
		if (entity.HasComponent<TransformComponent>())
		{
			if (ImGui::TreeNodeEx((void*)typeid(TransformComponent).hash_code(), ImGuiTreeNodeFlags_DefaultOpen, "Transform"))
			{
				auto& tc = entity.GetComponent<TransformComponent>();

				// 1. Position (永远允许修改)
				ImGui::DragFloat3("Position", glm::value_ptr(tc.Translation), 0.1f);

				// 2. Rotation (永远允许修改)
				glm::vec3 rotation = glm::degrees(tc.Rotation);
				if (ImGui::DragFloat3("Rotation", glm::value_ptr(rotation), 0.1f))
				{
					tc.Rotation = glm::radians(rotation);
				}

				// 3. Scale (根据是否是参数化 CAD 模型来决定是否锁定)
				bool isParametricCAD = false;
				if (entity.HasComponent<CADGeometryComponent>())
				{
					auto type = entity.GetComponent<CADGeometryComponent>().Type;
					// 如果是原生创建的几何体 (不是 None 也不是 Imported)
					if (type != CADGeometryComponent::GeometryType::None &&
						type != CADGeometryComponent::GeometryType::Imported)
					{
						isParametricCAD = true;
					}
				}

				if (isParametricCAD)
				{
					// --- 锁定模式 ---
					BeginDisabled(); // 变灰，禁止交互

					// 显示为 1.0 (Locked)
					glm::vec3 lockedScale(1.0f);
					ImGui::DragFloat3("Scale (Use Params)", glm::value_ptr(lockedScale));

					EndDisabled();

					// 强制把底层数据重置为 1 (防止之前被 Gizmo 意外修改过)
					tc.Scale = glm::vec3(1.0f);

					// 鼠标悬停提示
					if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
					{
						ImGui::SetTooltip("Scale is locked for Parametric Geometry.\nPlease edit Radius/Size in 'CAD Geometry' below.");
					}
				}
				else
				{
					// --- 普通模式 (允许缩放) ---
					ImGui::DragFloat3("Scale", glm::value_ptr(tc.Scale), 0.1f);
				}

				ImGui::TreePop();
			}
		}

		if (entity.HasComponent<CADGeometryComponent>())
		{
			if (ImGui::TreeNodeEx((void*)typeid(CADGeometryComponent).hash_code(), ImGuiTreeNodeFlags_DefaultOpen, "CAD Geometry"))
			{
				auto& cadComp = entity.GetComponent<CADGeometryComponent>();

				// 分别标记两种变化状态
				bool geometryChanged = false;   // 几何参数变了 (需要全套重建)
				bool meshQualityChanged = false; // 只有精度变了 (只需要重建网格)

				// 1. 几何参数滑块 (根据类型显示)
				if (cadComp.Type == CADGeometryComponent::GeometryType::Cube)
				{
					if (ImGui::DragFloat("Width", &cadComp.Params.Width, 0.1f, 0.001f, 10000.0f)) geometryChanged = true;
					if (ImGui::DragFloat("Height", &cadComp.Params.Height, 0.1f, 0.001f, 10000.0f)) geometryChanged = true;
					if (ImGui::DragFloat("Depth", &cadComp.Params.Depth, 0.1f, 0.001f, 10000.0f)) geometryChanged = true;
				}
				else if (cadComp.Type == CADGeometryComponent::GeometryType::Sphere)
				{
					if (ImGui::DragFloat("Radius", &cadComp.Params.Radius, 0.1f, 0.001f, 10000.0f)) geometryChanged = true;
				}
				else if (cadComp.Type == CADGeometryComponent::GeometryType::Cylinder)
				{
					if (ImGui::DragFloat("Radius", &cadComp.Params.Radius, 0.1f, 0.001f, 10000.0f)) geometryChanged = true;
					if (ImGui::DragFloat("Height", &cadComp.Params.Height, 0.1f, 0.001f, 10000.0f)) geometryChanged = true;
				}

				ImGui::Separator();

				// 2. 网格精度滑块 (所有 CAD 类型通用)
				// 范围：0.001 (极细) 到 10.0 (极粗)
				// 使用对数刻度或者普通浮点拖拽，为了明显，我们反直觉一点：
				// 数值越小越精细。
				if (ImGui::DragFloat("Mesh Quality", &cadComp.LinearDeflection, 0.01f, 0.001f, 10.0f, "%.3f"))
				{
					if (cadComp.LinearDeflection < 0.001f) cadComp.LinearDeflection = 0.001f;
					meshQualityChanged = true;
				}
				// 鼠标悬停提示
				if (ImGui::IsItemHovered())
				{
					ImGui::SetTooltip("Deflection Value: Smaller = Smoother (High CPU Cost)\nLarger = Coarser (Low CPU Cost)");
				}

				// ==========================================================
				// 边操作 UI (Fillet) 
				// ==========================================================
				if (m_selectedEdge != -1)
				{
					ImGui::Separator();
					ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "Selected Edge: %d", m_selectedEdge);

					static float s_CurrentFilletRadius = 0.2f;
					ImGui::DragFloat("Fillet Radius", &s_CurrentFilletRadius, 0.01f, 0.01f, 10.0f);

					// 绿色按钮
					ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.8f, 0.2f, 1.0f));
					if (ImGui::Button("Apply Fillet", ImVec2(-1, 0))) // 宽度填满
					{
						// 调用我们在 CADMesher 中实现的静态函数
						// 注意：ApplyFillet 内部已经包含了 RebuildMesh 的逻辑
						CADMesher::ApplyFillet(entity, m_selectedEdge, s_CurrentFilletRadius);

						// 操作完成后，重置选中状态，因为拓扑结构变了，旧 ID 失效
						m_selectedEdge = -1;
					}
					ImGui::PopStyleColor();
				}
				// ==========================================================

				// 3. 处理变化
				if (geometryChanged)
				{
					// 几何变了，必须重新 MakeShape -> Mesh
					RebuildCADGeometry(entity);
				}
				else if (meshQualityChanged)
				{
					// 只有精度变了，不需要重新 MakeShape，直接拿现有的 Shape 重新离散化
					// 这样效率更高，而且支持 Imported 模型 (因为 Imported 模型没有 Width/Height 参数可改，但有 Shape)
					RebuildMeshOnly(entity);
				}

				ImGui::TreePop();
			}
		}

		// --- Mesh  ---
		if (entity.HasComponent<MeshComponent>())
		{
			if (ImGui::TreeNodeEx((void*)typeid(MeshComponent).hash_code(), ImGuiTreeNodeFlags_DefaultOpen, "Mesh"))
			{
				auto& mesh = entity.GetComponent<MeshComponent>();
				ImGui::Text("Vertices: %zu", mesh.LocalVertices.size());
				ImGui::Text("Edges Mapped: %zu", mesh.m_IDToEdgeMap.size());
				ImGui::TreePop();
			}
		}
	}
}