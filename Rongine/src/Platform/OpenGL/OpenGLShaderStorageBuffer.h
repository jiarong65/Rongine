#pragma once
#include "Rongine/Renderer/ShaderStorageBuffer.h"

namespace Rongine {

	class OpenGLShaderStorageBuffer : public ShaderStorageBuffer
	{
	public:
		OpenGLShaderStorageBuffer(uint32_t size, ShaderStorageBufferUsage usage);
		virtual ~OpenGLShaderStorageBuffer();

		virtual void bind(uint32_t bindingPoint) const override;
		virtual void unbind() const override;

		virtual void setData(const void* data, uint32_t size, uint32_t offset = 0) override;

		virtual uint32_t getSize() const override { return m_size; }

		virtual void resize(uint32_t size) override;

	private:
		uint32_t m_rendererID;
		uint32_t m_size; 
		ShaderStorageBufferUsage m_usage;
	};
}