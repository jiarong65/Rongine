#include "Rongpch.h"
#include "OrthographicCameraController.h"
#include "Rongine/Core/Log.h"
#include "Rongine/Core/Input.h"
#include "Rongine/Core/KeyCodes.h"


namespace Rongine {
	Rongine::OrthographicCameraController::OrthographicCameraController(float aspectRatio, bool rotation)
		:m_aspectRatio(aspectRatio), 
		m_camera(-aspectRatio * m_zoomLevel, aspectRatio* m_zoomLevel, -m_zoomLevel, m_zoomLevel), 
		m_rotation(rotation)
	{

	}

	void OrthographicCameraController::onUpdate(Timestep ts)
	{
		//RONG_CORE_TRACE("Timestep is {0} ms", ts.getMilliseconds());

		if (Input::isKeyPressed(Key::A))
			m_position.x -= m_cameraTranslationSpeed * ts;
		else if (Rongine::Input::isKeyPressed(Rongine::Key::D))
			m_position.x += m_cameraTranslationSpeed * ts;
		if (Rongine::Input::isKeyPressed(Rongine::Key::W))
			m_position.y += m_cameraTranslationSpeed * ts;
		else if (Rongine::Input::isKeyPressed(Rongine::Key::S))
			m_position.y -= m_cameraTranslationSpeed * ts;

		if (m_rotation) 
		{
			if (Rongine::Input::isKeyPressed(Rongine::Key::Q))
				m_cameraRotation -= m_cameraRotationSpeed * ts;
			else if (Rongine::Input::isKeyPressed(Rongine::Key::E))
				m_cameraRotation += m_cameraRotationSpeed * ts;
		}

		m_camera.setPosition(m_position);
		m_camera.setRotation(m_cameraRotation);

		m_cameraTranslationSpeed = m_zoomLevel;
	}

	void OrthographicCameraController::onEvent(Event& e)
	{
		EventDispatcher dispatcher(e);
		dispatcher.dispatch<WindowResizeEvent>(RONG_BIND_EVENT_FN(OrthographicCameraController::onWindowResized));
		dispatcher.dispatch<MouseScrolledEvent>(RONG_BIND_EVENT_FN(OrthographicCameraController::onMouseScrolled));
	}

	bool OrthographicCameraController::onMouseScrolled(MouseScrolledEvent& e)
	{
		const auto& offset = e.getYOffset();
		m_zoomLevel -= offset * 0.25f;
		m_zoomLevel = std::max(m_zoomLevel, 0.25f);
		m_camera.setProjection(-m_aspectRatio * m_zoomLevel, m_aspectRatio * m_zoomLevel, -m_zoomLevel, m_zoomLevel);
		return false;
	}

	bool OrthographicCameraController::onWindowResized(WindowResizeEvent& e)
	{
		float width = e.getWidth();
		float height = e.getHeight();
		m_aspectRatio = width / height;
		m_camera.setProjection(-m_aspectRatio * m_zoomLevel, m_aspectRatio * m_zoomLevel, -m_zoomLevel, m_zoomLevel);
		return false;
	}
}

