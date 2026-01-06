#include "Rongpch.h"
#include "Framebuffer.h"
#include "Rongine/Renderer/Renderer.h"
#include "Rongine/Core/Log.h"
#include "Platform/OpenGL/OpenGLFramebuffer.h"

namespace Rongine {
	Ref<Framebuffer> Framebuffer::create(const FramebufferSpecification& spec)
	{
		switch (Renderer::getAPI())
		{
		case RendererAPI::API::None: {
			RONG_CORE_ASSERT(false, "RendererAPI::None is currently not supported!");
			return nullptr;
		}
		case RendererAPI::API::OpenGL: {
			return CreateRef<OpenGLFramebuffer>(spec);
		}
		}
		RONG_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}
}
