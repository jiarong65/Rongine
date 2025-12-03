#include "Rongpch.h"
#include "OpenGLContext.h"
#include <GLFW/glfw3.h>
#include <glad/glad.h>

namespace Rongine {
	void OpenGLContext::init() {
		glfwMakeContextCurrent(m_windowHandle);
		int status = gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

		RONG_CORE_ASSERT(status,"Failed to initialize Glad!");
	}

	void OpenGLContext::swapBuffers() {
		glfwSwapBuffers(m_windowHandle);
	}
}