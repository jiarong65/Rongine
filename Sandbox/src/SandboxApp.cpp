#include "Rongpch.h"
#include <Rongine.h>

#include <glm/vec3.hpp> // glm::vec3
#include <glm/vec4.hpp> // glm::vec4
#include <glm/mat4x4.hpp> // glm::mat4
#include <glm/gtc/matrix_transform.hpp> // glm::translate, glm::rotate, glm::scale, glm::perspective

#include "Platform/OpenGL/OpenGLShader.h"
#include "Rongine/Core/EntryPoint.h"
#include <glm/gtc/type_ptr.hpp>

#include "imgui/imgui.h"
#include "Sandbox2D.h"

class Sandbox :public Rongine::Application {
public:
	Sandbox() {
		//pushLayer(new ExampleLayer());
		pushLayer(new Sandbox2D());
		pushOverLayer(new Rongine::ImGuiLayer());
	}
	~Sandbox() {

	}
};

Rongine::Application* Rongine::createApplication()
{
	return new Sandbox;
}