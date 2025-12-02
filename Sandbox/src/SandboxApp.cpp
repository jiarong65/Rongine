#include "Rongpch.h"
#include <Rongine.h>

#include <glm/vec3.hpp> // glm::vec3
#include <glm/vec4.hpp> // glm::vec4
#include <glm/mat4x4.hpp> // glm::mat4
#include <glm/gtc/matrix_transform.hpp> // glm::translate, glm::rotate, glm::scale, glm::perspective

#include "imgui/imgui.h"
glm::mat4 camera(float Translate, glm::vec2 const& Rotate)
{
	glm::mat4 Projection = glm::perspective(glm::radians(45.0f), 4.0f / 3.0f, 0.1f, 100.f);
	glm::mat4 View = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -Translate));
	View = glm::rotate(View, Rotate.y, glm::vec3(-1.0f, 0.0f, 0.0f));
	View = glm::rotate(View, Rotate.x, glm::vec3(0.0f, 1.0f, 0.0f));
	glm::mat4 Model = glm::scale(glm::mat4(1.0f), glm::vec3(0.5f));
	return Projection * View * Model;
}

class ExampleLayer :public Rongine::Layer
{
public:
	ExampleLayer()
		:Layer("example")
	{
	}

	void onUpdate() override
	{
		//RONG_CLIENT_INFO("exampleLayer onUpdate");
		if (Rongine::Input::isKeyPressed(Rongine::Key::A))
		{
			RONG_CLIENT_TRACE("A key is pressed(POLLING)!");
		}
	}

	void onEvent(Rongine::Event& event)
	{
		//RONG_CLIENT_TRACE(event.toString());
		if (event.getEventType() == Rongine::EventType::KeyPressed)
		{
			Rongine::KeyPressedEvent& e = (Rongine::KeyPressedEvent&)event;
			if (e.getKeyCode() == Rongine::Key::A)
			{
				RONG_CLIENT_TRACE("A key event received(EVENT)!");
			}
		}
	}

	void onImGuiRender()
	{
		ImGui::Begin("test");
		ImGui::Text("hello world!!");
		ImGui::End();
	}
};

class Sandbox :public Rongine::Application {
public:
	Sandbox() {
		pushLayer(new ExampleLayer());
		pushOverLayer(new Rongine::ImGuiLayer());
	}
	~Sandbox() {

	}
};

Rongine::Application* Rongine::createApplication()
{
	return new Sandbox;
}