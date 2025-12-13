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

		inline static void setViewPort(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
		{
			s_rendererAPI->setViewPort(x, y, width, height);
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


