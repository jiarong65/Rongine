#include "Rongpch.h"
#include "OpenGLShaderStorageBuffer.h"
#include "Rongine/Core/Log.h"
#include <glad/glad.h>

namespace Rongine {

	static GLenum OpenGLUsage(ShaderStorageBufferUsage usage)
	{
		switch (usage)
		{
		case ShaderStorageBufferUsage::StaticDraw:  return GL_STATIC_DRAW;
		case ShaderStorageBufferUsage::DynamicDraw: return GL_DYNAMIC_DRAW;
		}
		RONG_CORE_ASSERT(false, "Unknown usage!");
		return GL_DYNAMIC_DRAW;
	}

	OpenGLShaderStorageBuffer::OpenGLShaderStorageBuffer(uint32_t size, ShaderStorageBufferUsage usage)
		: m_size(size), m_usage(usage)
	{
		glCreateBuffers(1, &m_rendererID);

		glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_rendererID);
		glBufferData(GL_SHADER_STORAGE_BUFFER, size, nullptr, OpenGLUsage(usage));
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
	}

	OpenGLShaderStorageBuffer::~OpenGLShaderStorageBuffer()
	{
		glDeleteBuffers(1, &m_rendererID);
	}

	void OpenGLShaderStorageBuffer::bind(uint32_t bindingPoint) const
	{
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, bindingPoint, m_rendererID);
	}

	void OpenGLShaderStorageBuffer::unbind() const
	{
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
	}

	void OpenGLShaderStorageBuffer::setData(const void* data, uint32_t size, uint32_t offset)
	{
		if (size + offset > m_size)
		{
			RONG_CORE_ERROR("ShaderStorageBuffer overflow! Trying to set {0} bytes at offset {1}, but capacity is {2}", size, offset, m_size);
			return; 
		}

		glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_rendererID);
		glBufferSubData(GL_SHADER_STORAGE_BUFFER, offset, size, data);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
	}

	void OpenGLShaderStorageBuffer::resize(uint32_t size)
	{
		m_size = size;
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_rendererID);

		glBufferData(GL_SHADER_STORAGE_BUFFER, size, nullptr, OpenGLUsage(m_usage));
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
	}
}