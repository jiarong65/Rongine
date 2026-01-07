#include "Rongpch.h"
#include "SceneHierarchyPanel.h"
#include "Rongine/Scene/Components.h"
#include "imgui.h"
#include <glm/gtc/type_ptr.hpp>

namespace Rongine {

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

				// 使用 DragFloat3 允许拖拽修改数值
				ImGui::DragFloat3("Position", glm::value_ptr(tc.Translation), 0.1f);

				// 欧拉角显示为度数，实际上存储为弧度
				glm::vec3 rotation = glm::degrees(tc.Rotation);
				if (ImGui::DragFloat3("Rotation", glm::value_ptr(rotation), 0.1f))
				{
					tc.Rotation = glm::radians(rotation);
				}

				ImGui::DragFloat3("Scale", glm::value_ptr(tc.Scale), 0.1f);

				ImGui::TreePop();
			}
		}

		// --- Mesh (只显示有没有) ---
		if (entity.HasComponent<MeshComponent>())
		{
			if (ImGui::TreeNodeEx((void*)typeid(MeshComponent).hash_code(), ImGuiTreeNodeFlags_DefaultOpen, "Mesh"))
			{
				ImGui::Text("Mesh Loaded");
				ImGui::TreePop();
			}
		}
	}
}