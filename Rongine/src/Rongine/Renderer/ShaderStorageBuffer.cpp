#include "Rongpch.h"
#include "ShaderStorageBuffer.h"
#include "Rongine/Renderer/Renderer.h"
#include "Platform/OpenGL/OpenGLShaderStorageBuffer.h"

namespace Rongine {

	Ref<ShaderStorageBuffer> ShaderStorageBuffer::create(uint32_t size, ShaderStorageBufferUsage usage)
	{
		switch (Renderer::getAPI())
		{
		case RendererAPI::API::None:    RONG_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
		case RendererAPI::API::OpenGL:  return CreateRef<OpenGLShaderStorageBuffer>(size, usage);
		}

		RONG_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}
}