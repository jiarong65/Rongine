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

		inline static void drawIndexed(const Ref<VertexArray>& vertexArray, uint32_t count)
		{
			s_rendererAPI->drawIndexed(vertexArray, count);
		}

		inline static void drawLines(const Ref<VertexArray>& vertexArray, uint32_t count)
		{
			s_rendererAPI->drawLines(vertexArray, count);
		}

		inline static void drawIndexedInstanced(const Ref<VertexArray>& vertexArray, uint32_t indexCount, uint32_t instanceCount)
		{
			s_rendererAPI->drawIndexedInstanced(vertexArray, indexCount, instanceCount);
		}

		inline static void drawArrays(const Ref<VertexArray>& vertexArray, uint32_t vertexCount)
		{
			s_rendererAPI->drawArrays(vertexArray, vertexCount);
		}

		inline static void setDepthTest(bool enabled)
		{
			s_rendererAPI->setDepthTest(enabled);
		}

		inline static void setDepthWrite(bool enabled)
		{
			s_rendererAPI->setDepthWrite(enabled);
		}

		inline static void setBlend(bool enabled)
		{
			s_rendererAPI->setBlend(enabled);
		}

		inline static void setCullFace(bool enabled, bool backFace = true)
		{
			s_rendererAPI->setCullFace(enabled, backFace);
		}

		inline static void setWireframe(bool enabled)
		{
			s_rendererAPI->setWireframe(enabled);
		}

	private:
		static RendererAPI* s_rendererAPI;
	};
}


