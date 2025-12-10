#pragma once
#include "Rongine/Renderer/RendererAPI.h"

namespace Rongine {
	class RenderCommand
	{
	public:
		inline static void init()
		{
			s_rendererAPI->init();
		}

		inline static void setColor(const glm::vec4& color)
		{
			s_rendererAPI->setColor(color);
		}
		inline static void clear()
		{
			s_rendererAPI->clear();
		}

		inline static void drawIndexed(const Ref<VertexArray>& vertexArray)
		{
			s_rendererAPI->drawIndexed(vertexArray);
		}
	private:
		static RendererAPI* s_rendererAPI;
	};
}


