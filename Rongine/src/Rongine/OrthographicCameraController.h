#pragma once
#include "Rongine/Renderer/OrthographicCamera.h"
#include "Rongine/Events/ApplicationEvent.h"
#include "Rongine/Events/MouseEvent.h"
#include "Rongine/Core/Timestep.h"

namespace Rongine {

	class OrthographicCameraController
	{
	public:
		OrthographicCameraController(float aspectRatio, bool rotation = false);
		
		void onUpdate(Timestep ts);
		void onEvent(Event& e);
		
		OrthographicCamera& getCamera() { return m_camera; }
		const OrthographicCamera& getCamera() const { return m_camera; }

		float getZoomLevel() const { return m_zoomLevel; }
		void setZoomLevel(float level)  { m_zoomLevel = level; }
	private:
		bool onMouseScrolled(MouseScrolledEvent& e);
		bool onWindowResized(WindowResizeEvent& e);
	private:
		float m_zoomLevel = 1.0f;
		OrthographicCamera m_camera;
		float m_aspectRatio;

		bool m_rotation;

		glm::vec3 m_position = { 0.0f,0.0f,0.0f };
		float m_cameraRotation = 0.0f;
		float m_cameraTranslationSpeed = 1.0f, m_cameraRotationSpeed = 180.0f;
	};

}

