#include "rongpch.h"
#include "Platform/Windows/WindowsWindow.h"

#include "Rongine/Events/ApplicationEvent.h"
#include "Rongine/Events/MouseEvent.h"
#include "Rongine/Events/KeyEvent.h"
#include "Rongine/Core/Log.h"
#include "Platform/OpenGL/OpenGLContext.h"

namespace Rongine {

    static uint8_t s_GLFWWindowCount = 0;

    static void GLFWErrorCallback(int error, const char* description)
    {
        // 你可加日志
    }

    WindowsWindow::WindowsWindow(const WindowProps& props)
    {
        Init(props);
    }

    WindowsWindow::~WindowsWindow()
    {
        Shutdown();
    }

    void WindowsWindow::Init(const WindowProps& props)
    {
        // 修复字段名
        m_Data.Title = props.m_title;
        m_Data.Width = props.m_width;
        m_Data.Height = props.m_height;


        if (s_GLFWWindowCount == 0)
        {
            int success = glfwInit();
            RONG_CORE_ASSERT(success, "Could not init glfw");
            glfwSetErrorCallback(GLFWErrorCallback);
        }

        // 创建窗口
        m_window = glfwCreateWindow(
            (int)props.m_width,
            (int)props.m_height,
            m_Data.Title.c_str(),
            nullptr, nullptr
        );
        
        m_context = new OpenGLContext(m_window);
		m_context->init();

        ++s_GLFWWindowCount;

        // 避免回调 nullptr crash
        //m_Data.eventCallBack = [](Event& e) {};

        glfwSetWindowUserPointer(m_window, &m_Data);
        setVSync(true);

        //==================== GLFW 回调 ====================//

        // --- 窗口大小变化 ---
        glfwSetWindowSizeCallback(m_window, [](GLFWwindow* window, int width, int height)
            {
                WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
                data.Width = width;
                data.Height = height;

                WindowResizeEvent event(width, height);
                data.eventCallBack(event);
            });

        // --- 窗口关闭 ---
        glfwSetWindowCloseCallback(m_window, [](GLFWwindow* window)
            {
                WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

                WindowCloseEvent event;
                data.eventCallBack(event);
            });

        // --- 键盘输入 ---
        glfwSetKeyCallback(m_window, [](GLFWwindow* window, int key, int scancode, int action, int mods)
            {
                WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

                switch (action)
                {
                case GLFW_PRESS:
                {
                    KeyPressedEvent event(key, 0);
                    data.eventCallBack(event);
                    break;
                }
                case GLFW_RELEASE:
                {
                    KeyReleasedEvent event(key);
                    data.eventCallBack(event);
                    break;
                }
                case GLFW_REPEAT:
                {
                    KeyPressedEvent event(key, true);
                    data.eventCallBack(event);
                    break;
                }
                }
            });

        // --- 字符输入 ---
        glfwSetCharCallback(m_window, [](GLFWwindow* window, unsigned int keycode)
            {
                WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

                KeyTypedEvent event(keycode);
                data.eventCallBack(event);
            });

        // --- 鼠标按键 ---
        glfwSetMouseButtonCallback(m_window, [](GLFWwindow* window, int button, int action, int mods)
            {
                WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

                switch (action)
                {
                case GLFW_PRESS:
                {
                    MouseButtonPressedEvent event(button);
                    data.eventCallBack(event);
                    break;
                }
                case GLFW_RELEASE:
                {
                    MouseButtonReleasedEvent event(button);
                    data.eventCallBack(event);
                    break;
                }
                }
            });

        // --- 鼠标滚轮 ---
        glfwSetScrollCallback(m_window, [](GLFWwindow* window, double xOffset, double yOffset)
            {
                WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

                MouseScrolledEvent event((float)xOffset, (float)yOffset);
                data.eventCallBack(event);
            });

        // --- 鼠标移动 ---
        glfwSetCursorPosCallback(m_window, [](GLFWwindow* window, double xPos, double yPos)
            {
                WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

                MouseMovedEvent event((float)xPos, (float)yPos);
                data.eventCallBack(event);
            });
    }

    void WindowsWindow::Shutdown()
    {
        glfwDestroyWindow(m_window);
        --s_GLFWWindowCount;

        if (s_GLFWWindowCount == 0)
        {
            glfwTerminate();
        }
    }

    void WindowsWindow::onUpdate()
    {
        glfwPollEvents();
        m_context->swapBuffers(); 
    }

    void WindowsWindow::setVSync(bool enabled)
    {
        glfwSwapInterval(enabled ? 1 : 0);
        m_Data.VSync = enabled;
    }

    bool WindowsWindow::isVSync() const
    {
        return m_Data.VSync;
    }

} // namespace Rongine
