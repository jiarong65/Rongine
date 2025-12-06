#pragma once

namespace Rongine {
	class VertexBuffer
	{
	public:
		virtual ~VertexBuffer() {};

		virtual void bind()const =0;
		virtual void unbind()const =0;
		
		static VertexBuffer* create(float* vertex,uint32_t size);
	};

	class IndexBuffer
	{
	public:
		virtual ~IndexBuffer() {};

		virtual void bind()const = 0;
		virtual void unbind()const = 0;

		virtual inline uint32_t getCount() const = 0;

		static IndexBuffer* create(uint32_t* indices, uint32_t count);
	};
}


