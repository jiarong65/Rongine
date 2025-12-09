#pragma once
#include "Rongine/Renderer/RendererAPI.h"

namespace Rongine {
	class OpenGLRendererAPI:public RendererAPI
	{
	public:
		virtual void setColor(const glm::vec4& color) override;
		virtual void clear()  override;

		virtual void drawIndexed(const Ref<VertexArray>& vertexArray) override;
	};

}

