#include "Rongpch.h"
#include "OpenGLRendererAPI.h"
#include <glad/glad.h>

namespace Rongine {
	void OpenGLRendererAPI::init()
	{
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}
	void OpenGLRendererAPI::setColor(const glm::vec4& color)
	{
		glClearColor(color.r,color.g,color.b,color.a);
	}
	void OpenGLRendererAPI::clear()
	{
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}
	void OpenGLRendererAPI::drawIndexed(const Ref<VertexArray>& vertexArray)
	{
		glDrawElements(GL_TRIANGLES, vertexArray->getIndexBuffer()->getCount(), GL_UNSIGNED_INT, nullptr);
	}
}
