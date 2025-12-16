#include "Rongpch.h"
#include <Rongine.h>
#include "Sandbox2D.h"
#include "Platform/OpenGL/OpenGLShader.h"
#include "Rongine/Core/Input.h"
#include "Rongine/Core/KeyCodes.h"
#include <glm/gtc/type_ptr.hpp>

#include "imgui/imgui.h"

Sandbox2D::Sandbox2D()
	:Layer("Sandbox2D"), m_cameraContorller(1280.0f/720.0f)
{
}

void Sandbox2D::onAttach()
{
	m_checkerboardTexture = Rongine::Texture2D::create("assets/textures/Checkerboard.png");
}

void Sandbox2D::onDetach()
{
}

void Sandbox2D::onUpdate(Rongine::Timestep ts)
{
	m_cameraContorller.onUpdate(ts);

	if (Rongine::Input::isKeyPressed(Rongine::Key::J))
		m_squarePosition.x -= m_squareMovedSpeed * ts;
	else if (Rongine::Input::isKeyPressed(Rongine::Key::L))
		m_squarePosition.x += m_squareMovedSpeed * ts;
	if (Rongine::Input::isKeyPressed(Rongine::Key::I))
		m_squarePosition.y += m_squareMovedSpeed * ts;
	else if (Rongine::Input::isKeyPressed(Rongine::Key::K))
		m_squarePosition.y -= m_squareMovedSpeed * ts;

	Rongine::RenderCommand::setColor({ 0.1f, 0.1f, 0.1f, 1 });
	Rongine::RenderCommand::clear();

	Rongine::Renderer2D::beginScene(m_cameraContorller.getCamera());
	Rongine::Renderer2D::drawQuad({ 0.0f, 0.0f, -0.1f }, { 10.0f, 10.0f }, m_checkerboardTexture);
	Rongine::Renderer2D::drawQuad(m_squarePosition, { 1.0f, 1.0f }, m_squareColor);
	Rongine::Renderer2D::drawQuad({ 0.5f, -0.5f }, { 0.5f, 0.75f }, { 0.2f, 0.3f, 0.8f, 1.0f });
	Rongine::Renderer2D::endScene();
}

void Sandbox2D::onImGuiRender()
{
	ImGui::Begin("Setting");
	ImGui::ColorEdit3("Square Color", glm::value_ptr(m_squareColor));
	ImGui::End();
}

void Sandbox2D::onEvent(Rongine::Event& e)
{
	m_cameraContorller.onEvent(e);
}
