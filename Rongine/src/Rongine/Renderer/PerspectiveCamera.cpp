#include "Rongpch.h"
#include "PerspectiveCamera.h"
#include <glm/gtc/matrix_transform.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp> // 用于处理旋转

namespace Rongine {

	PerspectiveCamera::PerspectiveCamera(float fov, float aspectRatio, float nearClip, float farClip)
		: m_projectionMatrix(glm::perspective(glm::radians(fov), aspectRatio, nearClip, farClip)), m_viewMatrix(1.0f)
	{
		m_viewProjectionMatrix = m_projectionMatrix * m_viewMatrix;
	}

	void PerspectiveCamera::setProjection(float fov, float aspectRatio, float nearClip, float farClip)
	{
		m_projectionMatrix = glm::perspective(glm::radians(fov), aspectRatio, nearClip, farClip);
		m_viewProjectionMatrix = m_projectionMatrix * m_viewMatrix;
	}

	void PerspectiveCamera::recalculateViewMatrix()
	{
		// 既然是 3D 相机，用 Quaternion (四元数) 构造旋转矩阵是更稳健的做法
		// 这里的 Pitch 是绕 X 轴，Yaw 是绕 Y 轴
		glm::quat orientation = glm::quat(glm::vec3(-m_pitch, -m_yaw, 0.0f));

		// 构造变换矩阵：先平移，再旋转
		glm::mat4 transform = glm::translate(glm::mat4(1.0f), m_position) * glm::toMat4(orientation);

		// View 矩阵是相机的逆变换
		m_viewMatrix = glm::inverse(transform);
		m_viewProjectionMatrix = m_projectionMatrix * m_viewMatrix;
	}

	glm::vec3 PerspectiveCamera::getForwardDirection() const
	{
		// 根据 Pitch 和 Yaw 计算前方向量
		return glm::rotate(glm::quat(glm::vec3(-m_pitch, -m_yaw, 0.0f)), glm::vec3(0.0f, 0.0f, -1.0f));
	}

	glm::vec3 PerspectiveCamera::getRightDirection() const
	{
		return glm::rotate(glm::quat(glm::vec3(-m_pitch, -m_yaw, 0.0f)), glm::vec3(1.0f, 0.0f, 0.0f));
	}

	glm::vec3 PerspectiveCamera::getUpDirection() const
	{
		return glm::rotate(glm::quat(glm::vec3(-m_pitch, -m_yaw, 0.0f)), glm::vec3(0.0f, 1.0f, 0.0f));
	}
}