#pragma once
#include "Rongine/Renderer/UniformBuffer.h"

namespace Rongine {

	class OpenGLUniformBuffer : public UniformBuffer
	{
	public:
		OpenGLUniformBuffer(uint32_t size, uint32_t bindingPoint);
		virtual ~OpenGLUniformBuffer();

		virtual void setData(const void* data, uint32_t size, uint32_t offset = 0) override;
		virtual void bind(uint32_t bindingPoint) const override;

	private:
		uint32_t m_rendererID = 0;
	};

}
