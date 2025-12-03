#include "Rongpch.h"
#include "OpenGLContext.h"
#include <GLFW/glfw3.h>
#include <glad/glad.h>

namespace Rongine {
	void OpenGLContext::init() {
		glfwMakeContextCurrent(m_windowHandle);
		int status = gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

		RONG_CORE_ASSERT(status,"Failed to initialize Glad!");

		RONG_CORE_INFO("OpenGL Info:");
		RONG_CORE_INFO("  Vendor: {0}", (const char*)glGetString(GL_VENDOR));
		RONG_CORE_INFO("  Renderer: {0}",(const char*) glGetString(GL_RENDERER));
		RONG_CORE_INFO("  Version: {0}",(const char*)glGetString(GL_VERSION));
	}

	void OpenGLContext::swapBuffers() {
		glfwSwapBuffers(m_windowHandle);
	}
}