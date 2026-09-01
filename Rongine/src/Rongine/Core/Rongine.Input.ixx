module;

#include <cstdint>
#include <utility>

#include "Rongine/Core/RongineMacros.h"
#include <GLFW/glfw3.h>

export module Rongine.Input;

export import Rongine.Core;
export import Rongine.Application;
export import Rongine.Window;

export namespace Rongine {

	class RONG_API Input
	{
	public:
		static bool isKeyPressed(KeyCode key);
		static bool isMouseButtonPressed(MouseCode button);
		static std::pair<float, float> getMousePosition();
		static float getMouseX();
		static float getMouseY();
	};

	bool Input::isKeyPressed(KeyCode key)
	{
		auto window = static_cast<GLFWwindow*>(Application::get().getWindow().getNativeWindow());
		auto state = glfwGetKey(window, static_cast<int32_t>(key));
		return state == GLFW_PRESS || state == GLFW_REPEAT;
	}

	bool Input::isMouseButtonPressed(MouseCode button)
	{
		auto window = static_cast<GLFWwindow*>(Application::get().getWindow().getNativeWindow());
		auto state = glfwGetMouseButton(window, static_cast<int32_t>(button));
		return state == GLFW_PRESS;
	}

	std::pair<float, float> Input::getMousePosition()
	{
		auto window = static_cast<GLFWwindow*>(Application::get().getWindow().getNativeWindow());
		double xpos, ypos;
		glfwGetCursorPos(window, &xpos, &ypos);
		return { (float)xpos, (float)ypos };
	}

	float Input::getMouseX()
	{
		auto [x, y] = getMousePosition();
		return x;
	}

	float Input::getMouseY()
	{
		auto [x, y] = getMousePosition();
		return y;
	}
}
