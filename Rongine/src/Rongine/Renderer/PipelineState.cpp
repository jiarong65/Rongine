#include "Rongpch.h"
#include "PipelineState.h"
#include "Rongine/Renderer/RendererAPI.h"
#include "Platform/OpenGL/OpenGLPipelineState.h"

namespace Rongine {

	Ref<PipelineState> PipelineState::create(const PipelineStateDesc& desc)
	{
		switch (RendererAPI::getAPI())
		{
		case RendererAPI::API::OpenGL:
			return CreateRef<OpenGLPipelineState>(desc);
		}
		RONG_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}

}
