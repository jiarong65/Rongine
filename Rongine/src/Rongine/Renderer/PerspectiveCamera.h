#pragma once

#include <glm/glm.hpp>

namespace Rongine {

	class PerspectiveCamera
	{
	public:
		// fov: 视野角度 (度), width/height: 宽高比, near/far: 近/远裁剪面
		PerspectiveCamera(float fov, float aspectRatio, float nearClip, float farClip);
		~PerspectiveCamera() = default;

		const glm::vec3& getPosition() const { return m_position; }
		void setPosition(const glm::vec3& position) { m_position = position; recalculateViewMatrix(); }

		// 3D 中通常使用 Pitch(上下看) 和 Yaw(左右看)
		float getPitch() const { return m_pitch; }
		float getYaw() const { return m_yaw; }
		void setRotation(float pitch, float yaw) { m_pitch = pitch; m_yaw = yaw; recalculateViewMatrix(); }

		void setProjection(float fov, float aspectRatio, float nearClip, float farClip);

		const glm::mat4& getProjectionMatrix() const { return m_projectionMatrix; }
		const glm::mat4& getViewMatrix() const { return m_viewMatrix; }
		const glm::mat4& getViewProjectionMatrix() const { return m_viewProjectionMatrix; }

		// 获取相机的方向向量 (用于移动计算)
		glm::vec3 getForwardDirection() const;
		glm::vec3 getRightDirection() const;
		glm::vec3 getUpDirection() const;

	private:
		void recalculateViewMatrix();

	private:
		glm::mat4 m_projectionMatrix;
		glm::mat4 m_viewMatrix;
		glm::mat4 m_viewProjectionMatrix;

		glm::vec3 m_position = { 0.0f, 0.0f, 0.0f };

		// 欧拉角 (Euler Angles)
		float m_pitch = 0.0f;
		float m_yaw = 0.0f; // 初始化朝向，通常 0 或者 -90 度取决于坐标系定义
	};

}