#include "Rongpch.h"
#include "SceneHierarchyPanel.h"
#include "Rongine/Scene/Components.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "Rongine/CAD/CADModeler.h"
#include "Rongine/CAD/CADMesher.h"
#include "Rongine/CAD/CADImporter.h"
#include "Rongine/Commands/Command.h" 
#include "Rongine/Commands/CADModifyCommand.h" 
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
			std::vector<uint32_t> newIndices;

			// 调用 Mesher 生成新数据
			auto newVA = CADMesher::CreateMeshFromShape(*occShape, newVertices,newIndices,cadComp.LinearDeflection);

			if (newVA)
			{
				// 4. 更新 MeshComponent
				meshComp.VA = newVA;
				meshComp.LocalVertices = newVertices; // 更新 CPU 顶点，保证 Gizmo 吸附正确
				meshComp.LocalIndices = newIndices;
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
			std::vector<uint32_t> newIndices;

			// 传入当前的 精度
			auto newVA = CADMesher::CreateMeshFromShape(*occShape, newVertices, newIndices, cadComp.LinearDeflection);

			if (newVA)
			{
				meshComp.VA = newVA;
				meshComp.LocalVertices = newVertices;
				meshComp.LocalIndices = newIndices;
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
			if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
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
			}
		}

		if (entity.HasComponent<CADGeometryComponent>())
		{
			if (ImGui::CollapsingHeader("CAD Geometry", ImGuiTreeNodeFlags_DefaultOpen))
			{
				auto& cadComp = entity.GetComponent<CADGeometryComponent>();

				// --- 1. 参数化几何滑块 (带 Undo/Redo) ---
				auto DrawParamSlider = [&](const char* label, float* value, float minVal = 0.001f)
					{
						// 使用静态变量加锁，防止 Width 的状态影响 Height/Depth
						static CADModifyCommand* s_ActiveCommand = nullptr;
						static const char* s_ActiveLabel = nullptr; // 记录当前正在操作哪个滑块

						// 先绘制滑块，ImGui 的状态函数检测的是“刚刚提交的 Item”
						bool valueChanged = ImGui::DragFloat(label, value, 0.1f, minVal, 10000.0f);

						// 2. 检测激活 (鼠标刚按下)
						if (ImGui::IsItemActivated())
						{
							// 安全清理（防止异常状态残留）
							if (s_ActiveCommand) { delete s_ActiveCommand; s_ActiveCommand = nullptr; }

							s_ActiveCommand = new CADModifyCommand(entity); // 备份旧状态
							s_ActiveLabel = label; // 锁定当前滑块
						}

						// 3. 处理逻辑 (仅当当前滑块是激活者时)
						if (s_ActiveCommand && s_ActiveLabel == label)
						{
							// 拖拽中且值改变：刷新预览
							if (ImGui::IsItemActive() && valueChanged)
							{
								RebuildCADGeometry(entity);
							}

							// 鼠标松开：提交或废弃
							if (ImGui::IsItemDeactivated())
							{
								if (ImGui::IsItemDeactivatedAfterEdit())
								{
									// 值确实变了：提交到历史栈
									s_ActiveCommand->CaptureNewState();
									CommandHistory::Push(s_ActiveCommand);
									s_ActiveCommand = nullptr; // 归属权移交

									if (m_SceneChangedCallback)
										m_SceneChangedCallback();
								}
								else
								{
									// 只是点了一下没改值：删除临时命令
									delete s_ActiveCommand;
									s_ActiveCommand = nullptr;
								}
								s_ActiveLabel = nullptr; // 解锁
							}
						}
					};

				// 根据类型绘制滑块
				if (cadComp.Type == CADGeometryComponent::GeometryType::Cube)
				{
					DrawParamSlider("Width", &cadComp.Params.Width);
					DrawParamSlider("Height", &cadComp.Params.Height);
					DrawParamSlider("Depth", &cadComp.Params.Depth);
				}
				else if (cadComp.Type == CADGeometryComponent::GeometryType::Sphere)
				{
					DrawParamSlider("Radius", &cadComp.Params.Radius);
				}
				else if (cadComp.Type == CADGeometryComponent::GeometryType::Cylinder)
				{
					DrawParamSlider("Radius", &cadComp.Params.Radius);
					DrawParamSlider("Height", &cadComp.Params.Height);
				}

				ImGui::Separator();

				// --- 2. 网格精度滑块 (无 Undo) ---
				bool meshQualityChanged = false;
				if (ImGui::DragFloat("Mesh Quality", &cadComp.LinearDeflection, 0.01f, 0.001f, 10.0f, "%.3f"))
				{
					cadComp.LinearDeflection = std::max(cadComp.LinearDeflection, 0.001f);
					meshQualityChanged = true;

					if (m_SceneChangedCallback)
						m_SceneChangedCallback();
				}
				if (ImGui::IsItemHovered())
					ImGui::SetTooltip("Deflection Value: Smaller = Smoother (High CPU Cost)\nLarger = Coarser (Low CPU Cost)");

				if (meshQualityChanged) RebuildMeshOnly(entity);

				// --- 3. 倒角操作 (带 Undo/Redo) ---
				if (m_selectedEdge != -1)
				{
					ImGui::Separator();
					ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "Selected Edge: %d", m_selectedEdge);

					static float s_CurrentFilletRadius = 0.2f;
					ImGui::DragFloat("Fillet Radius", &s_CurrentFilletRadius, 0.01f, 0.01f, 10.0f);

					ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.8f, 0.2f, 1.0f));
					if (ImGui::Button("Apply Fillet", ImVec2(-1, 0)))
					{
						// A. 备份旧状态
						auto* cmd = new CADModifyCommand(entity);

						// B. 执行操作
						CADMesher::ApplyFillet(entity, m_selectedEdge, s_CurrentFilletRadius);

						// C. 备份新状态并提交
						cmd->CaptureNewState();
						CommandHistory::Push(cmd);

						// D. 强制取消选择 (防止拓扑变动导致崩溃)
						m_selectedEdge = -1;
					}
					ImGui::PopStyleColor();

					if (m_SceneChangedCallback)
						m_SceneChangedCallback();
				}
			}
		}

		// --- Mesh  ---
		if (entity.HasComponent<MeshComponent>())
		{
			if (ImGui::CollapsingHeader("Mesh", ImGuiTreeNodeFlags_DefaultOpen))
			{
				auto& mesh = entity.GetComponent<MeshComponent>();
				ImGui::Text("Vertices: %zu", mesh.LocalVertices.size());
				ImGui::Text("Edges Mapped: %zu", mesh.m_IDToEdgeMap.size());
			}
		}

		// --- Materials  ---
		if (entity.HasComponent<MaterialComponent>())
		{
			auto& mat = entity.GetComponent<MaterialComponent>();
			if (ImGui::CollapsingHeader("Material", ImGuiTreeNodeFlags_DefaultOpen))
			{
				bool changed = false;

				// 如果存在光谱组件，普通 RGB 颜色会被覆盖，给个提示并禁用编辑
				bool hasSpectral = entity.HasComponent<SpectralMaterialComponent>();

				if (hasSpectral)
				{
					ImGui::TextColored(ImVec4(0.2f, 0.8f, 0.2f, 1.0f), "[Overridden by Spectral Data]");
					BeginDisabled(); // 禁用 RGB 编辑
				}

				if (ImGui::ColorEdit3("Albedo", (float*)&mat.Albedo)) changed = true;

				if (hasSpectral) EndDisabled(); // 恢复可用

				// 粗糙度和金属度通常是通用的，即使在光谱模式下也有效
				if (ImGui::SliderFloat("Roughness", &mat.Roughness, 0.0f, 1.0f)) changed = true;
				if (ImGui::SliderFloat("Metallic", &mat.Metallic, 0.0f, 1.0f)) changed = true;

				//  添加升级按钮
				if (!hasSpectral)
				{
					ImGui::Spacing();
					ImGui::Separator();
					if (ImGui::Button("Upgrade to Spectral Material", ImVec2(-1, 0)))
					{
						auto& newSpec = entity.AddComponent<SpectralMaterialComponent>("New Spectral");

						// 初始化一个默认的平坦光谱 (防止空数据崩溃或不显示)
						// 32 个采样点，反射率 0.8 (浅灰色)
						newSpec.SpectrumValues.resize(32, 0.8f);

						if (m_SceneChangedCallback) m_SceneChangedCallback();
					}
				}

				if (changed && m_SceneChangedCallback)
				{
					m_SceneChangedCallback();
				}
			}
		}

		if (entity.HasComponent<SpectralMaterialComponent>())
		{
			if (ImGui::CollapsingHeader("Spectral Material", ImGuiTreeNodeFlags_DefaultOpen))
			{
				auto& specComp = entity.GetComponent<SpectralMaterialComponent>();

				// 1. 材质名称修改
				char buffer[256];
				memset(buffer, 0, sizeof(buffer));
				strcpy_s(buffer, sizeof(buffer), specComp.Name.c_str());
				if (ImGui::InputText("Spec Name", buffer, sizeof(buffer)))
				{
					specComp.Name = std::string(buffer);
				}

				// 2. 曲线可视化 (X轴: 380-780nm, Y轴: 反射率)
				if (!specComp.SpectrumValues.empty())
				{
					ImGui::Text("Reflectance Curve (380nm - 780nm)");
					ImGui::PlotLines("##Spectrum",
						specComp.SpectrumValues.data(),
						(int)specComp.SpectrumValues.size(),
						0,
						NULL,
						0.0f, 1.0f, // Y轴范围 0~1
						ImVec2(0, 80)); // 高度 80px
				}
				else
				{
					ImGui::TextColored(ImVec4(1, 0, 0, 1), "No spectral data available.");
				}

				ImGui::Spacing();

				// 3. 移除按钮 (回退到 RGB)
				if (ImGui::Button("Remove Spectral Data", ImVec2(-1, 0)))
				{
					entity.RemoveComponent<SpectralMaterialComponent>();
					if (m_SceneChangedCallback) m_SceneChangedCallback();
				}
			}
		}
	}
}