#pragma once
#include "Rongine/Renderer/GraphicsContext.h"
#include "Rongine/Log.h"

struct GLFWwindow;

namespace Rongine {
	class OpenGLContext:public GraphicsContext
	{
	public:
		OpenGLContext(GLFWwindow* windowHandle)
			:m_windowHandle(windowHandle)
		{
			RONG_CORE_ASSERT(m_windowHandle, "window handle is null");
		}

		virtual void init() override;
		virtual void swapBuffers() override;
	private:
		GLFWwindow* m_windowHandle;
	};

}

