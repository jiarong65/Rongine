#include "Rongpch.h"
#include "ComputeShader.h"
#include "Rongine/Renderer/Renderer.h"
#include "Platform/OpenGL/OpenGLComputeShader.h"

namespace Rongine {

	Ref<ComputeShader> ComputeShader::create(const std::string& filepath)
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
			return CreateRef<OpenGLComputeShader>(filepath);
		}
		}
		RONG_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}
}