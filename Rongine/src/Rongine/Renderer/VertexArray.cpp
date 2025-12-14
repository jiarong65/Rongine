#include "Rongpch.h"
#include "VertexArray.h"
#include "Renderer.h"
#include "Platform/OpenGL/OpenGLVertexArray.h"

namespace Rongine {
	Ref<VertexArray> Rongine::VertexArray::create()
	{
		switch (Renderer::getAPI())
		{
		case RendererAPI::API::None: 
		{
			RONG_CORE_ASSERT(false, "RendererAPI::None is currently not supported!");
			return nullptr;
		}
		case RendererAPI::API::OpenGL:
		{
			return std::make_shared<OpenGLVertexArray>();
		}
		}
		RONG_CORE_ASSERT(false, "UnKnown RendererAPI!");
		return nullptr;
	}
}


