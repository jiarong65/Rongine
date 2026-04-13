#include "Rongpch.h"
#include "UniformBuffer.h"
#include "Rongine/Renderer/RendererAPI.h"
#include "Platform/OpenGL/OpenGLUniformBuffer.h"

namespace Rongine {

	Ref<UniformBuffer> UniformBuffer::create(uint32_t size, uint32_t bindingPoint)
	{
		switch (RendererAPI::getAPI())
		{
		case RendererAPI::API::OpenGL:
			return CreateRef<OpenGLUniformBuffer>(size, bindingPoint);
		}
		RONG_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}

}
