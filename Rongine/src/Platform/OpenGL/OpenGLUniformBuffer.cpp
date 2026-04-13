#include "Rongpch.h"
#include "OpenGLUniformBuffer.h"
#include <glad/glad.h>

namespace Rongine {

	OpenGLUniformBuffer::OpenGLUniformBuffer(uint32_t size, uint32_t bindingPoint)
	{
		glCreateBuffers(1, &m_rendererID);
		glNamedBufferData(m_rendererID, size, nullptr, GL_DYNAMIC_DRAW);
		glBindBufferBase(GL_UNIFORM_BUFFER, bindingPoint, m_rendererID);
	}

	OpenGLUniformBuffer::~OpenGLUniformBuffer()
	{
		glDeleteBuffers(1, &m_rendererID);
	}

	void OpenGLUniformBuffer::setData(const void* data, uint32_t size, uint32_t offset)
	{
		glNamedBufferSubData(m_rendererID, offset, size, data);
	}

	void OpenGLUniformBuffer::bind(uint32_t bindingPoint) const
	{
		glBindBufferBase(GL_UNIFORM_BUFFER, bindingPoint, m_rendererID);
	}

}
