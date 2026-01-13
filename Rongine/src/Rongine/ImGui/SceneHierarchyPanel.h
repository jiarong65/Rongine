#pragma once

#include "Rongine/Core/Core.h"
#include "Rongine/Scene/Scene.h"
#include "Rongine/Scene/Entity.h"

namespace Rongine {

	class SceneHierarchyPanel
	{
	public:
		SceneHierarchyPanel() = default;
		SceneHierarchyPanel(const Ref<Scene>& context);

		void setContext(const Ref<Scene>& context);

		void onImGuiRender();

		Entity getSelectedEntity() const { return m_selectionContext; }
		void setSelectedEntity(Entity entity);

		void setSelectedEdge(int edgeID) { m_selectedEdge = edgeID; }
		int getSelectedEdge() const { return m_selectedEdge; }

	private:
		void drawEntityNode(Entity entity);
		void drawComponents(Entity entity);

	private:
		Ref<Scene> m_context;
		Entity m_selectionContext; // 当前在面板里选中的物体

		int m_selectedEdge=-2;		//当前选中的边
	};
}