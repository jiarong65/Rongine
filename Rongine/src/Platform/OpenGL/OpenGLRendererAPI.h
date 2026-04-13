#pragma once
#include "Rongine/Renderer/RendererAPI.h"

namespace Rongine {
	class OpenGLRendererAPI : public RendererAPI
	{
	public:
		virtual void init() override;
		virtual void setColor(const glm::vec4& color) override;
		virtual void setViewPort(uint32_t x, uint32_t y, uint32_t width, uint32_t height) override;
		virtual void clear() override;

		virtual void drawIndexed(const Ref<VertexArray>& vertexArray, uint32_t count = 0) override;
		virtual void drawLines(const Ref<VertexArray>& vertexArray, uint32_t vertexCount) override;

		virtual void drawIndexedInstanced(const Ref<VertexArray>& vertexArray, uint32_t indexCount, uint32_t instanceCount) override;
		virtual void drawArrays(const Ref<VertexArray>& vertexArray, uint32_t vertexCount) override;

		virtual void setDepthTest(bool enabled) override;
		virtual void setDepthWrite(bool enabled) override;
		virtual void setBlend(bool enabled) override;
		virtual void setCullFace(bool enabled, bool backFace = true) override;
		virtual void setWireframe(bool enabled) override;
	};

}

