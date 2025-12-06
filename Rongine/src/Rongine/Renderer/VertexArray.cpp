#include "Rongpch.h"
#include "VertexArray.h"
#include "Renderer.h"
#include "Platform/OpenGL/OpenGLVertexArray.h"

namespace Rongine {
	VertexArray* Rongine::VertexArray::create()
	{
		switch (Renderer::getAPI())
		{
		case RendererAPI::None: 
		{
			RONG_CORE_ASSERT(false, "RendererAPI::None is currently not supported!");
			return nullptr;
		}
		case RendererAPI::OpenGL:
		{
			return new OpenGLVertexArray();
		}
		}
	}
}


