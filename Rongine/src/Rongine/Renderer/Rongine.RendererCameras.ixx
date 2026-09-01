module;
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

export module Rongine.RendererCameras;

export namespace Rongine {

	class OrthographicCamera
	{
	public:
		OrthographicCamera(float left, float right, float bottom, float top);
		~OrthographicCamera() = default;

		const glm::vec3& getPosition() const { return m_position; }
		void setPosition(const glm::vec3& position) { recalculateViewMatrix(); m_position = position; }
		void setProjection(float left, float right, float bottom, float top);

		float getRotation() const { return m_rotation; }
		void setRotation(float rotation) { recalculateViewMatrix(); m_rotation = rotation; }

		const glm::mat4& getProjectionMatrix() const { return m_projectionMatrix; }
		const glm::mat4& getViewMatrix() const { return m_viewMatrix; }
		const glm::mat4& getViewProjectionMatrix() const { return m_viewProjectionMatrix; }
	private:
		void recalculateViewMatrix();
	private:
		glm::mat4 m_projectionMatrix;
		glm::mat4 m_viewMatrix;
		glm::mat4 m_viewProjectionMatrix;

		glm::vec3 m_position = { 0.0f,0.0f,0.0f };
		float m_rotation = 0.0f;
	};

	class PerspectiveCamera
	{
	public:
		PerspectiveCamera(float fov, float aspectRatio, float nearClip, float farClip);
		~PerspectiveCamera() = default;

		const glm::vec3& getPosition() const { return m_position; }
		void setPosition(const glm::vec3& position) { m_position = position; recalculateViewMatrix(); }

		float getPitch() const { return m_pitch; }
		float getYaw() const { return m_yaw; }
		void setRotation(float pitch, float yaw) { m_pitch = pitch; m_yaw = yaw; recalculateViewMatrix(); }

		void setProjection(float fov, float aspectRatio, float nearClip, float farClip);

		const glm::mat4& getProjectionMatrix() const { return m_projectionMatrix; }
		const glm::mat4& getViewMatrix() const { return m_viewMatrix; }
		const glm::mat4& getViewProjectionMatrix() const { return m_viewProjectionMatrix; }

		glm::mat4 getInverseViewMatrix() const { return glm::inverse(m_viewMatrix); }
		glm::mat4 getInverseProjectionMatrix() const { return glm::inverse(m_projectionMatrix); }

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

		float m_pitch = 0.0f;
		float m_yaw = 0.0f;
	};

	OrthographicCamera::OrthographicCamera(float left, float right, float bottom, float top)
		: m_projectionMatrix(glm::ortho(left, right, bottom, top, -1.0f, 1.0f)), m_viewMatrix(1.0f)
	{
		m_viewProjectionMatrix = m_projectionMatrix * m_viewMatrix;
	}

	void OrthographicCamera::setProjection(float left, float right, float bottom, float top)
	{
		m_projectionMatrix = glm::ortho(left, right, bottom, top, -1.0f, 1.0f);
		m_viewProjectionMatrix = m_projectionMatrix * m_viewMatrix;
	}

	void OrthographicCamera::recalculateViewMatrix()
	{
		glm::mat4 transform = glm::translate(glm::mat4(1.0f), m_position) *
			glm::rotate(glm::mat4(1.0f), glm::radians(m_rotation), glm::vec3(0.0f, 0.0f, 1.0f));

		m_viewMatrix = glm::inverse(transform);
		m_viewProjectionMatrix = m_projectionMatrix * m_viewMatrix;
	}

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
		glm::quat orientation = glm::quat(glm::vec3(-m_pitch, -m_yaw, 0.0f));

		glm::mat4 transform = glm::translate(glm::mat4(1.0f), m_position) * glm::toMat4(orientation);

		m_viewMatrix = glm::inverse(transform);
		m_viewProjectionMatrix = m_projectionMatrix * m_viewMatrix;
	}

	glm::vec3 PerspectiveCamera::getForwardDirection() const
	{
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
