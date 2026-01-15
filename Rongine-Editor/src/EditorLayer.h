#pragma once
#include <Rongine.h>
#include "Rongine/Core/Layer.h"
#include "Rongine/Renderer/Shader.h"
#include "Rongine/Renderer/VertexArray.h"
#include "Rongine/Renderer/OrthographicCameraController.h"
#include "Rongine/Renderer/PerspectiveCameraController.h"
#include "Rongine/Renderer/Renderer.h"
#include "Rongine/Core/Timestep.h"
#include "Rongine/Scene/Scene.h"
#include "Rongine/Scene/Entity.h"

#include <glm/glm.hpp>

class EditorLayer :public Rongine::Layer
{
public:
	EditorLayer();
	virtual ~EditorLayer() = default;

	virtual void onAttach() override;
	virtual void onDetach() override;
	virtual void onUpdate(Rongine::Timestep ts) override;
	virtual void onImGuiRender() override;
	virtual void onEvent(Rongine::Event& e) override;

private:
	void ImportSTEP();
	void CreatePrimitive(Rongine::CADGeometryComponent::GeometryType type);
	void SaveSceneAs();
	void OpenScene();
	void OnBooleanOperation(Rongine::CADBoolean::Operation op);
	void EnterExtrudeMode();
	void ExitExtrudeMode(bool apply);   // apply=true(融合), false(取消)

	void EnterFilletMode();
	void ExitFilletMode(bool apply);

private:
	Rongine::Ref<Rongine::Shader> m_shader;
	//Rongine::OrthographicCameraController m_cameraContorller;
	Rongine::PerspectiveCameraController m_cameraContorller;

	Rongine::Ref<Rongine::VertexArray> m_vertexArray;
	//Rongine::Ref<Rongine::VertexArray> m_CadMeshVA, m_TorusVA;;
	Rongine::Ref<Rongine::Texture2D> m_checkerboardTexture;
	Rongine::Ref<Rongine::Texture2D> m_logoTexture;

	Rongine::Ref<Rongine::Framebuffer> m_framebuffer;

	struct ProfileResult
	{
		const char* name;
		float time;
	};
	std::vector<ProfileResult> m_profileResult;

	glm::vec4 m_squareColor = { 0.2f, 0.3f, 0.8f, 1.0f };
	glm::vec3 m_squarePosition = { 0.0f,0.0f,0.0f };

	float m_squareMovedSpeed = 3.0f;

	bool m_viewportFocused = false, m_viewportHovered = false;

	glm::vec2 m_viewportSize = { 0.0f, 0.0f };

	glm::vec2 m_viewportBounds[2];

	Rongine::Ref<Rongine::Scene> m_activeScene;
	Rongine::Entity m_selectedEntity; // 使用 Entity 对象
	int m_selectedFace = -1;
	int m_selectedEdge = -1;
	int m_gizmoType = -1; // -1:None, 0:Translate, 1:Rotate, 2:Scale

	int m_HoveredEntityID = -1;
	int m_HoveredFaceID = -1;
	int m_HoveredEdgeID = -1;

	Rongine::SceneHierarchyPanel m_sceneHierarchyPanel;

	Rongine::Entity m_ToolEntity;                //工具实体

	// --- Gizmo 撤销状态 ---
	bool m_GizmoEditing = false; // 是否正在拖拽中
	Rongine::TransformComponent m_GizmoStartTransform; // 拖拽开始时的快照

	// --- 拉伸模式状态 ---
	bool m_IsExtrudeMode = false;       // 是否处于拉伸模式
	float m_ExtrudeHeight = 0.0f;       // 当前拉伸高度
	glm::mat4 m_ExtrudeGizmoMatrix;     // Gizmo 的初始位置矩阵
	Rongine::Entity m_PreviewEntity;    // 用于显示拉伸预览的临时实体

	// --- 倒角模式状态 ---
	bool m_IsFilletMode = false;
	float m_FilletRadius = 0.0f;
	int m_FilletEdge;
	glm::mat4 m_FilletGizmoMatrix = glm::mat4(1.0f);
};

