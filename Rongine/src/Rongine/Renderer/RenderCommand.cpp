#include "Rongpch.h"
#include "RenderCommand.h"
#include "Platform/OpenGL/OpenGLRendererAPI.h"

namespace Rongine {

	RendererAPI* RenderCommand::s_rendererAPI = new OpenGLRendererAPI;
}
