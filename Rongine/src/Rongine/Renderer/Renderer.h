#pragma once
#include "Rongine/Renderer/RendererAPI.h"
#include "Rongine/Renderer/RenderCommand.h"

namespace Rongine {


	class Renderer
	{
	public:
		static void beginScene();
		static void endScene();

		static void submit(const std::shared_ptr<VertexArray>& vertexArray);

		inline static RendererAPI::API getAPI() { return RendererAPI::getAPI(); }	
	};

}


