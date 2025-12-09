#include "Rongpch.h"
#include "Texture.h"
#include "Rongine/Renderer/Renderer.h"
#include "Platform/OpenGL/OpenGLTexture.h"

namespace Rongine {

	Ref<Texture2D> Rongine::Texture2D::create(const std::string& path)
	{
		switch (Renderer::getAPI())
		{
		case RendererAPI::API::None: {
			RONG_CORE_ASSERT(false, "RendererAPI::None is currently not supported!");
			return nullptr;
		}
		case RendererAPI::API::OpenGL: {
			return std::make_shared<OpenGLTexture2D>(path);
		}
		}
		RONG_CORE_ASSERT(false,"Unknown RendererAPI!");
		return nullptr;
	}
}


