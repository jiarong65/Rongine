#include "Rongpch.h"
#include <Rongine.h>
#include "Sandbox2D.h"
#include "Platform/OpenGL/OpenGLShader.h"
#include "Rongine/Core/Input.h"
#include "Rongine/Core/KeyCodes.h"
#include <glm/gtc/type_ptr.hpp>
#include <chrono>

#include "imgui/imgui.h"

template<typename Fn>
class Timer
{
public:
	Timer(const char* name, Fn&& func)
		:m_name(name), m_func(func),m_stopped(false)
	{
		m_startTimepoint = std::chrono::high_resolution_clock::now();
	}

	~Timer()
	{
		if (!m_stopped)
			stop();
	}

	void stop()
	{
		auto endTimepoint = std::chrono::high_resolution_clock::now();

		long long start = std::chrono::time_point_cast<std::chrono::microseconds>(m_startTimepoint).time_since_epoch().count();
		long long end = std::chrono::time_point_cast<std::chrono::microseconds>(endTimepoint).time_since_epoch().count();

		float duration = (end - start)*0.001;
		m_func({ m_name,duration });
	}
private:
	const char* m_name;
	Fn m_func;
	std::chrono::time_point<std::chrono::steady_clock> m_startTimepoint;
	bool m_stopped;
};

#define PROFILE_SCOPE(name) Timer timer##__LINE__(name,[this](ProfileResult result){m_profileResult.push_back(result);});


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
	PROFILE_SCOPE("Sandbox2D::OnUpdate");

	{
		PROFILE_SCOPE("CameraContorller::OnUpdate");
		m_cameraContorller.onUpdate(ts);
	}

	{
		PROFILE_SCOPE("Sandbox2D::Input");
		if (Rongine::Input::isKeyPressed(Rongine::Key::J))
			m_squarePosition.x -= m_squareMovedSpeed * ts;
		else if (Rongine::Input::isKeyPressed(Rongine::Key::L))
			m_squarePosition.x += m_squareMovedSpeed * ts;
		if (Rongine::Input::isKeyPressed(Rongine::Key::I))
			m_squarePosition.y += m_squareMovedSpeed * ts;
		else if (Rongine::Input::isKeyPressed(Rongine::Key::K))
			m_squarePosition.y -= m_squareMovedSpeed * ts;
	}

	{
		PROFILE_SCOPE("Renderer Prep");
		Rongine::RenderCommand::setColor({ 0.1f, 0.1f, 0.1f, 1 });
		Rongine::RenderCommand::clear();
	}
	

	{
		PROFILE_SCOPE("Renderer Draw");
		Rongine::Renderer2D::beginScene(m_cameraContorller.getCamera());
		/*Rongine::Renderer2D::drawRotatedQuad({ 0.0f, 0.0f, -0.1f }, { 10.0f, 10.0f },45.0f, m_checkerboardTexture,10.0f,glm::vec4(1.0f,0.8f,0.8f,0.5f));
		Rongine::Renderer2D::drawRotatedQuad(m_squarePosition, { 1.0f, 1.0f }, 60.0f,m_squareColor);*/
		Rongine::Renderer2D::drawQuad({ 0.5f, -0.5f }, { 0.5f, 0.75f }, { 0.2f, 0.3f, 0.8f, 1.0f });
		Rongine::Renderer2D::drawQuad(m_squarePosition, { 1.0f, 1.0f },  m_squareColor);
		Rongine::Renderer2D::endScene();
	}

}

void Sandbox2D::onImGuiRender()
{
	ImGui::Begin("Setting");
	ImGui::ColorEdit3("Square Color", glm::value_ptr(m_squareColor));
	for (auto& result : m_profileResult)
	{
		char label[50];
		strcpy(label, "%.3fms  ");
		strcat(label, result.name);
		ImGui::Text(label, result.time);
	}
	m_profileResult.clear();
	ImGui::End();
}

void Sandbox2D::onEvent(Rongine::Event& e)
{
	m_cameraContorller.onEvent(e);
}
