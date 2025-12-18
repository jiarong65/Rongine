#pragma once
#include "Rongine/Renderer/RendererAPI.h"

namespace Rongine {
	class OpenGLRendererAPI:public RendererAPI
	{
	public:
		virtual void init() override;
		virtual void setColor(const glm::vec4& color) override;
		virtual void setViewPort(uint32_t x, uint32_t y, uint32_t width, uint32_t height) override;
		virtual void clear()  override;

		virtual void drawIndexed(const Ref<VertexArray>& vertexArray,uint32_t count=0) override;
	};

}

