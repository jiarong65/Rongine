#include "Rongpch.h"
#include "Buffer.h"
#include "Renderer.h"
#include "Rongine/Log.h"
#include "Platform/OpenGL/OpenGLBuffer.h"


namespace Rongine {
	
	////////////////////////VertexBuffer/////////////////////////////


	VertexBuffer* VertexBuffer::create(float* vertex, uint32_t size)
	{
		switch (Renderer::getAPI())
		{
		case RendererAPI::API::None: {
			RONG_CORE_ASSERT(false, "RendererAPI::None is currently not supported!");
			return nullptr;
		}
		case RendererAPI::API::OpenGL: {
			return new OpenGLVertexBuffer(vertex, size);
		}
		}
		RONG_CORE_ASSERT(false, "UnKnown RendererAPI!");
		return nullptr;
	}

	////////////////////////IndexBuffer/////////////////////////////

	IndexBuffer* IndexBuffer::create(uint32_t* indices, uint32_t count)
	{
		switch (Renderer::getAPI())
		{
		case RendererAPI::API::None: {
			RONG_CORE_ASSERT(false, "RendererAPI::None is currently not supported!");
			return nullptr;
		}
		case RendererAPI::API::OpenGL: {
			return new OpenGLIndexBuffer(indices, count);
		}
		}
		RONG_CORE_ASSERT(false, "UnKnown RendererAPI!");
		return nullptr;
	} 

}