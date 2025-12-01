#include "Rongpch.h"
#include <Rongine.h>

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
};

class Sandbox :public Rongine::Application {
public:
	Sandbox() {
		pushLayer(new ExampleLayer());
		//pushOverLayer(new Rongine::ImGuiLayer());
	}
	~Sandbox() {

	}
};

Rongine::Application* Rongine::createApplication()
{
	return new Sandbox;
}