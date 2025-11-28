#pragma once

#include "Rongine/Window.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>

namespace Rongine {

	class WindowsWindow : public Window
	{
	public:
		WindowsWindow(const WindowProps& props);
		virtual ~WindowsWindow();

		void onUpdate() override;

		unsigned int getWidth() const override { return m_Data.Width; }
		unsigned int getHeight() const override { return m_Data.Height; }

		// Window attributes
		void setEventCallBack(const eventCallBackFn& callback) override { m_Data.eventCallBack = callback; }
		void setVSync(bool enabled) override;
		bool isVSync() const override;

		virtual void* getNativeWindow() const { return m_window; }
	private:
		virtual void Init(const WindowProps& props);
		virtual void Shutdown();
	private:
		GLFWwindow* m_window;
		//Scope<GraphicsContext> m_Context;

		struct WindowData
		{
			std::string Title;
			unsigned int Width, Height;
			bool VSync;

			eventCallBackFn eventCallBack;
		};

		WindowData m_Data;
	};

}