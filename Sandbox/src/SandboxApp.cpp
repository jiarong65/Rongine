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
	}

	void onEvent(Rongine::Event& event)
	{
		RONG_CLIENT_TRACE(event.toString());
	}
};

class Sandbox :public Rongine::Application {
public:
	Sandbox() {
		pushLayer(new ExampleLayer());
	}
	~Sandbox() {

	}
};

Rongine::Application* Rongine::createApplication()
{
	return new Sandbox;
}