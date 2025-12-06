#pragma once
#include "Rongine/Renderer/Buffer.h"

namespace Rongine {
	class OpenGLVertexBuffer:public VertexBuffer
	{
	public:
		OpenGLVertexBuffer(float* vertex,uint32_t size) ;
		virtual ~OpenGLVertexBuffer();

		virtual void bind() const override;
		virtual void unbind() const override;
	private:
		uint32_t m_rendererID;
	};

	class OpenGLIndexBuffer :public IndexBuffer
	{
	public:
		OpenGLIndexBuffer(uint32_t* indices, uint32_t count);
		virtual ~OpenGLIndexBuffer();

		inline uint32_t getCount() const override{ return m_count; }
		virtual void bind() const override;
		virtual void unbind() const override;
	private:
		uint32_t m_rendererID;
		uint32_t m_count;
	};

}

