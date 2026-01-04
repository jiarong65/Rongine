#include "Rongpch.h"
#include "PerspectiveCameraController.h"
#include "Rongine/Core/Input.h"
#include "Rongine/Core/KeyCodes.h"
#include "Rongine/Core/MouseCodes.h" 

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace Rongine {

	PerspectiveCameraController::PerspectiveCameraController(float fov, float aspectRatio)
		: m_FOV(fov), m_aspectRatio(aspectRatio),
		m_camera(fov, aspectRatio, m_nearClip, m_farClip)
	{
		m_distance = 10.0f;
		m_focalPoint = { 0.0f, 0.0f, 0.0f };
		m_yaw = 0.0f;
		m_pitch = 0.0f;

		// 【修复1】：构造时初始化鼠标位置，防止程序刚启动第一帧计算出巨大的 delta
		m_initialMousePosition = { Input::getMouseX(), Input::getMouseY() };

		updateView();
	}

	void PerspectiveCameraController::onUpdate(Timestep ts)
	{
		// 1. 获取鼠标信息 & 计算 Delta
		glm::vec2 mouse = { Input::getMouseX(), Input::getMouseY() };
		glm::vec2 delta = (mouse - m_initialMousePosition) * 0.003f; // 0.003 是鼠标灵敏度
		m_initialMousePosition = mouse;

		// 2. WASD 移动 (移动的是焦点 FocalPoint)
		if (Input::isKeyPressed(Key::W))
			m_focalPoint += getForwardDirection() * m_moveSpeed * (float)ts;
		if (Input::isKeyPressed(Key::S))
			m_focalPoint -= getForwardDirection() * m_moveSpeed * (float)ts;
		if (Input::isKeyPressed(Key::A))
			m_focalPoint -= getRightDirection() * m_moveSpeed * (float)ts;
		if (Input::isKeyPressed(Key::D))
			m_focalPoint += getRightDirection() * m_moveSpeed * (float)ts;

		// Q/E 垂直升降
		if (Input::isKeyPressed(Key::E))
			m_focalPoint += getUpDirection() * m_moveSpeed * (float)ts;
		if (Input::isKeyPressed(Key::Q))
			m_focalPoint -= getUpDirection() * m_moveSpeed * (float)ts;

		// 3. 处理 Alt + 左键旋转 (Orbit)
		if (Input::isKeyPressed(Key::LeftAlt) && Input::isMouseButtonPressed(0))
		{
			if (!m_isRotating)
			{
				// --- 状态进入：刚按下的一瞬间 ---
				m_isRotating = true;

				// 【方案 A 核心】：
				// 这里不再调用 checkMouseRayIntersection 来改变 m_focalPoint。
				// 我们保持焦点不变，让用户围绕当前的中心旋转。

				// 【修复2】：忽略按下这一帧的 Delta
				// 防止因为上一帧鼠标位置差异导致的瞬间跳跃
				delta = glm::vec2(0.0f);
			}

			// 执行旋转 (围绕原有的 m_focalPoint)
			mouseRotate(delta);
		}
		else
		{
			// 松开左键，重置状态
			m_isRotating = false;
		}

		// 4. 处理中键平移 (Pan)
		if (Input::isMouseButtonPressed(2))
		{
			mousePan(delta);
		}

		// 5. 应用所有计算
		updateView();
	}

	void PerspectiveCameraController::onEvent(Event& e)
	{
		EventDispatcher dispatcher(e);
		dispatcher.dispatch<MouseScrolledEvent>(RONG_BIND_EVENT_FN(PerspectiveCameraController::onMouseScrolled));
		dispatcher.dispatch<WindowResizeEvent>(RONG_BIND_EVENT_FN(PerspectiveCameraController::onWindowResized));
	}

	bool PerspectiveCameraController::onMouseScrolled(MouseScrolledEvent& e)
	{
		float delta = e.getYOffset() * 0.1f;
		mouseZoom(delta);
		updateView();
		return false;
	}

	bool PerspectiveCameraController::onWindowResized(WindowResizeEvent& e)
	{
		m_viewportWidth = (float)e.getWidth();
		m_viewportHeight = (float)e.getHeight();

		if (m_viewportWidth > 0 && m_viewportHeight > 0)
		{
			m_aspectRatio = m_viewportWidth / m_viewportHeight;
			m_camera.setProjection(m_FOV, m_aspectRatio, m_nearClip, m_farClip);
		}
		return false;
	}

	void PerspectiveCameraController::onResize(float width, float height)
	{
		m_viewportWidth = width;
		m_viewportHeight = height;
		m_aspectRatio = width / height;
		m_camera.setProjection(m_FOV, m_aspectRatio, m_nearClip, m_farClip);
	}

	// ---------------- 核心功能实现 ----------------

	// 注意：虽然旋转不再调用此函数，但保留它用于后续开发（如选择物体）
	bool PerspectiveCameraController::checkMouseRayIntersection(const glm::vec2& mousePos, glm::vec3& outIntersection)
	{
		float x = (2.0f * mousePos.x) / m_viewportWidth - 1.0f;
		float y = 1.0f - (2.0f * mousePos.y) / m_viewportHeight;

		glm::vec4 rayClip = glm::vec4(x, y, -1.0, 1.0);
		glm::vec4 rayEye = glm::inverse(m_camera.getProjectionMatrix()) * rayClip;
		rayEye = glm::vec4(rayEye.x, rayEye.y, -1.0, 0.0);
		glm::vec3 rayWorld = glm::vec3(glm::inverse(m_camera.getViewMatrix()) * rayEye);
		rayWorld = glm::normalize(rayWorld);

		glm::vec3 rayOrigin = m_camera.getPosition();
		glm::vec3 planeNormal = { 0.0f, 1.0f, 0.0f };
		glm::vec3 planePoint = { 0.0f, 0.0f, 0.0f };

		float denom = glm::dot(planeNormal, rayWorld);

		if (std::abs(denom) > 0.0001f)
		{
			float t = glm::dot(planePoint - rayOrigin, planeNormal) / denom;
			if (t >= 0.0f && t < 1000.0f) // 适当的距离保护
			{
				outIntersection = rayOrigin + rayWorld * t;
				return true;
			}
		}

		return false;
	}

	void PerspectiveCameraController::mousePan(const glm::vec2& delta)
	{
		auto [xSpeed, ySpeed] = getPanSpeed();
		m_focalPoint += -getRightDirection() * delta.x * xSpeed * m_distance;
		m_focalPoint += getUpDirection() * delta.y * ySpeed * m_distance;
	}

	void PerspectiveCameraController::mouseRotate(const glm::vec2& delta)
	{
		float yawSign = getUpDirection().y < 0 ? -1.0f : 1.0f;
		m_yaw += yawSign * delta.x * m_rotationSpeed;
		m_pitch += delta.y * m_rotationSpeed;
	}

	void PerspectiveCameraController::mouseZoom(float delta)
	{
		m_distance -= delta * m_distance;
		if (m_distance < 1.0f)
		{
			m_focalPoint += getForwardDirection();
			m_distance = 1.0f;
		}
	}

	void PerspectiveCameraController::updateView()
	{
		// 1. 先计算位置
		m_position = m_focalPoint - getForwardDirection() * m_distance;

		// 2. 根据 Pitch/Yaw 计算旋转四元数
		glm::quat orientation = getOrientation();

		// 3. 将位置和旋转应用到 Camera 对象
		m_camera.setPosition(m_position);
		// 注意：PerspectiveCamera 内部可能用的是 ViewMatrix = inverse(Translate * Rotate)
		// 确保这里的 setRotation 逻辑与你的 PerspectiveCamera 实现匹配
		m_camera.setRotation(m_pitch, m_yaw);
	}

	std::pair<float, float> PerspectiveCameraController::getPanSpeed() const
	{
		float x = std::min(m_viewportWidth / 1000.0f, 2.4f);
		float xFactor = 0.0366f * (x * x) - 0.1778f * x + 0.3021f;

		float y = std::min(m_viewportHeight / 1000.0f, 2.4f);
		float yFactor = 0.0366f * (y * y) - 0.1778f * y + 0.3021f;

		return { xFactor, yFactor };
	}

	glm::quat PerspectiveCameraController::getOrientation() const
	{
		return glm::quat(glm::vec3(-m_pitch, -m_yaw, 0.0f));
	}

	glm::vec3 PerspectiveCameraController::getUpDirection() const
	{
		return glm::rotate(getOrientation(), glm::vec3(0.0f, 1.0f, 0.0f));
	}

	glm::vec3 PerspectiveCameraController::getRightDirection() const
	{
		return glm::rotate(getOrientation(), glm::vec3(1.0f, 0.0f, 0.0f));
	}

	glm::vec3 PerspectiveCameraController::getForwardDirection() const
	{
		return glm::rotate(getOrientation(), glm::vec3(0.0f, 0.0f, -1.0f));
	}
}