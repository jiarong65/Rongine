#pragma once

#include "PerspectiveCamera.h"
#include "Rongine/Core/Timestep.h"
#include "Rongine/Events/ApplicationEvent.h"
#include "Rongine/Events/MouseEvent.h"

#include <glm/glm.hpp>

namespace Rongine {

	class PerspectiveCameraController
	{
	public:
		PerspectiveCameraController(float fov, float aspectRatio);

		void onUpdate(Timestep ts);
		void onEvent(Event& e);
		void onResize(float width, float height);

		PerspectiveCamera& getCamera() { return m_camera; }
		const PerspectiveCamera& getCamera() const { return m_camera; }

		float getDistance() const { return m_distance; }
		void setDistance(float distance) { m_distance = distance; }

		void setFocus(const glm::vec3& focalPoint, float distance);

	private:
		bool onMouseScrolled(MouseScrolledEvent& e);
		bool onWindowResized(WindowResizeEvent& e);

		void mousePan(const glm::vec2& delta);
		void mouseRotate(const glm::vec2& delta);
		void mouseZoom(float delta);

		void updateView();
		std::pair<float, float> getPanSpeed() const;

		// 保留这个辅助函数，未来做“鼠标拾取物体”时非常有用
		bool checkMouseRayIntersection(const glm::vec2& mousePos, glm::vec3& outIntersection);

		glm::vec3 getUpDirection() const;
		glm::vec3 getRightDirection() const;
		glm::vec3 getForwardDirection() const;
		glm::quat getOrientation() const;

	private:
		float m_aspectRatio;
		float m_FOV = 45.0f;
		float m_nearClip = 0.1f, m_farClip = 1000.0f;

		PerspectiveCamera m_camera;

		glm::vec3 m_focalPoint = { 0.0f, 0.0f, 0.0f };
		glm::vec3 m_position = { 0.0f, 0.0f, 10.0f };

		float m_distance = 10.0f;
		float m_pitch = 0.0f;
		float m_yaw = 0.0f;

		float m_moveSpeed = 5.0f;
		float m_rotationSpeed = 0.8f;

		glm::vec2 m_initialMousePosition = { 0.0f, 0.0f };

		bool m_isRotating = false; // 用于处理按下瞬间的状态

		float m_viewportWidth = 1280.0f;
		float m_viewportHeight = 720.0f;
	};
}