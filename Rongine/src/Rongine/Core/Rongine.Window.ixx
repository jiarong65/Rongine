module;
#include <cstdint>
#include <functional>
#include <string>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "Rongine/Core/RongineMacros.h"

export module Rongine.Window;

export import Rongine.Core;
export import Rongine.Events;
export import Rongine.Renderer;

export namespace Rongine {

	struct WindowProps {
		uint32_t m_width;
		uint32_t m_height;
		std::string m_title;

		WindowProps(const std::string title = "Rongine",
			uint32_t w = 1600,
			uint32_t h = 900)
			: m_title(title), m_width(w), m_height(h) {}
	};

	class Window {
	public:
		using eventCallBackFn = std::function<void(Event&)>;
		virtual ~Window() = default;

		virtual void onUpdate() = 0;
		virtual uint32_t getWidth() const = 0;
		virtual uint32_t getHeight() const = 0;

		virtual void setEventCallBack(const eventCallBackFn& fn) = 0;
		virtual void setVSync(bool enabled) = 0;
		virtual bool isVSync() const = 0;

		virtual void pollEvents() = 0;
		virtual void swapBuffers() = 0;

		virtual void* getNativeWindow() const = 0;

		static Window* create(const WindowProps& props = WindowProps());
	};

	class WindowsWindow : public Window
	{
	public:
		WindowsWindow(const WindowProps& props);
		virtual ~WindowsWindow();

		void onUpdate() override;

		unsigned int getWidth() const override { return m_Data.Width; }
		unsigned int getHeight() const override { return m_Data.Height; }

		void setEventCallBack(const eventCallBackFn& callback) override { m_Data.eventCallBack = callback; }
		void setVSync(bool enabled) override;
		bool isVSync() const override;

		virtual void pollEvents() override;
		virtual void swapBuffers() override;

		virtual void* getNativeWindow() const override { return m_window; }
	private:
		virtual void Init(const WindowProps& props);
		virtual void Shutdown();

	private:
		GLFWwindow* m_window;
		GraphicsContext* m_context;

		struct WindowData
		{
			std::string Title;
			unsigned int Width, Height;
			bool VSync;

			eventCallBackFn eventCallBack;
		};

		WindowData m_Data;
	};

	Window* Window::create(const WindowProps& props)
	{
#ifdef RONG_PLATFORM_WINDOWS
		return new WindowsWindow(props);
#else
#error RONGINE ONLY SUPPORT WINDOWS
#endif
	}

	inline uint8_t s_GLFWWindowCount = 0;

	inline void GLFWErrorCallback(int error, const char* description)
	{
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
		m_Data.Title = props.m_title;
		m_Data.Width = props.m_width;
		m_Data.Height = props.m_height;

		if (s_GLFWWindowCount == 0)
		{
			int success = glfwInit();
			RONG_CORE_ASSERT(success, "Could not init glfw");
			glfwSetErrorCallback(GLFWErrorCallback);
		}

		m_window = glfwCreateWindow(
			(int)props.m_width,
			(int)props.m_height,
			m_Data.Title.c_str(),
			nullptr, nullptr
		);

		m_context = new OpenGLContext(m_window);
		m_context->init();

		++s_GLFWWindowCount;

		glfwSetWindowUserPointer(m_window, &m_Data);
		setVSync(true);

		glfwSetWindowSizeCallback(m_window, [](GLFWwindow* window, int width, int height)
			{
				WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
				data.Width = width;
				data.Height = height;

				WindowResizeEvent event(width, height);
				data.eventCallBack(event);
			});

		glfwSetWindowCloseCallback(m_window, [](GLFWwindow* window)
			{
				WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

				WindowCloseEvent event;
				data.eventCallBack(event);
			});

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

		glfwSetCharCallback(m_window, [](GLFWwindow* window, unsigned int keycode)
			{
				WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

				KeyTypedEvent event(keycode);
				data.eventCallBack(event);
			});

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

		glfwSetScrollCallback(m_window, [](GLFWwindow* window, double xOffset, double yOffset)
			{
				WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

				MouseScrolledEvent event((float)xOffset, (float)yOffset);
				data.eventCallBack(event);
			});

		glfwSetCursorPosCallback(m_window, [](GLFWwindow* window, double xPos, double yPos)
			{
				WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

				MouseMovedEvent event((float)xPos, (float)yPos);
				data.eventCallBack(event);
			});
	}

	void WindowsWindow::Shutdown()
	{
		glfwMakeContextCurrent(nullptr);
		glfwDestroyWindow(m_window);
		m_window = nullptr;
		--s_GLFWWindowCount;

		if (s_GLFWWindowCount == 0)
		{
			glfwTerminate();
		}
	}

	void WindowsWindow::onUpdate()
	{
		pollEvents();
		swapBuffers();
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

	void WindowsWindow::pollEvents()
	{
		glfwPollEvents();
	}

	void WindowsWindow::swapBuffers()
	{
		m_context->swapBuffers();
	}
}
