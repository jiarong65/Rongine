#include "Rongpch.h"
#include <Rongine.h>

class Sandbox :public Rongine::Application {
public:
	Sandbox() {

	}
	~Sandbox() {

	}
};

Rongine::Application* Rongine::createApplication()
{
	return new Sandbox;
}