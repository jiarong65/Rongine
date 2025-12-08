#include "Rongpch.h"
#include "Shader.h"


#include "Rongine/Log.h"
#include "Rongine/Renderer/Renderer.h"
#include "Platform/OpenGL/OpenGLShader.h"

namespace Rongine {


	Shader* Shader::create(const std::string& vertexSrc, const std::string& fragmentSrc)
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
			return new OpenGLShader(vertexSrc,fragmentSrc);
		}

		RONG_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
		}
	}


}

