#pragma once
#include <Rongine.h>
#include "Rongine/Core/Layer.h"
#include "Rongine/Renderer/Shader.h"
#include "Rongine/Renderer/VertexArray.h"
#include "Rongine/Renderer/OrthographicCameraController.h"
#include "Rongine/Renderer/PerspectiveCameraController.h"
#include "Rongine/Renderer/Renderer.h"
#include "Rongine/Core/Timestep.h"

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
	Rongine::Ref<Rongine::Shader> m_shader;
	//Rongine::OrthographicCameraController m_cameraContorller;
	Rongine::PerspectiveCameraController m_cameraContorller;

	Rongine::Ref<Rongine::VertexArray> m_vertexArray;
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
};

