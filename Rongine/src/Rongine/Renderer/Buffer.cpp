#include "Rongpch.h"
#include "Buffer.h"
#include "Renderer.h"
#include "Rongine/Core/Log.h"
#include "Platform/OpenGL/OpenGLBuffer.h"


namespace Rongine {
	
	////////////////////////VertexBuffer/////////////////////////////


	Ref<VertexBuffer> VertexBuffer::create(uint32_t size)
	{

		switch (Renderer::getAPI())
		{
		case RendererAPI::API::None: {
			RONG_CORE_ASSERT(false, "RendererAPI::None is currently not supported!");
			return nullptr;
		}
		case RendererAPI::API::OpenGL: {
			return CreateRef<OpenGLVertexBuffer>(size);
		}
		}
		RONG_CORE_ASSERT(false, "UnKnown RendererAPI!");
		return nullptr;
	}

	Ref<VertexBuffer> VertexBuffer::create(float* vertex, uint32_t size)
	{
		switch (Renderer::getAPI())
		{
		case RendererAPI::API::None: {
			RONG_CORE_ASSERT(false, "RendererAPI::None is currently not supported!");
			return nullptr;
		}
		case RendererAPI::API::OpenGL: {
			return CreateRef<OpenGLVertexBuffer>(vertex, size);
		}
		}
		RONG_CORE_ASSERT(false, "UnKnown RendererAPI!");
		return nullptr;
	}

	////////////////////////IndexBuffer/////////////////////////////

	Ref<IndexBuffer> IndexBuffer::create(uint32_t count)
	{
		switch (Renderer::getAPI())
		{
		case RendererAPI::API::None: {
			RONG_CORE_ASSERT(false, "RendererAPI::None is currently not supported!");
			return nullptr;
		}
		case RendererAPI::API::OpenGL: {
			return CreateRef<OpenGLIndexBuffer>(count);
		}
		}
		RONG_CORE_ASSERT(false, "UnKnown RendererAPI!");
		return nullptr;
	}

	Ref<IndexBuffer> IndexBuffer::create(uint32_t* indices, uint32_t count)
	{
		switch (Renderer::getAPI())
		{
		case RendererAPI::API::None: {
			RONG_CORE_ASSERT(false, "RendererAPI::None is currently not supported!");
			return nullptr;
		}
		case RendererAPI::API::OpenGL: {
			return CreateRef<OpenGLIndexBuffer>(indices, count);
		}
		}
		RONG_CORE_ASSERT(false, "UnKnown RendererAPI!");
		return nullptr;
	} 

}