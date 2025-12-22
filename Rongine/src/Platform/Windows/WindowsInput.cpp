#include "Rongpch.h"
#include "Rongine/Core/Input.h"
#include "Rongine/Core/Application.h"
#include "Rongine/Core/KeyCodes.h"
#include "Rongine/Core/MouseCodes.h"
#include <GLFW/glfw3.h>

namespace Rongine
{
    // 直接实现 Input 类中的静态方法
    bool Input::isKeyPressed(KeyCode key)
    {
        // 获取原生窗口句柄
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
        auto [x, y] = getMousePosition(); // 使用 C++17 结构化绑定
        return x;
    }

    float Input::getMouseY()
    {
        auto [x, y] = getMousePosition();
        return y;
    }
}