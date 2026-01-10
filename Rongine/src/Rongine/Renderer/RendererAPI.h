#pragma once
#include <glm/glm.hpp>
#include "Rongine/Renderer/VertexArray.h"

namespace Rongine {

	class RendererAPI
	{
	public:
		enum class API {
			None = 0, OpenGL = 1
		};
	public:
		virtual void init() = 0;
		virtual void setColor(const glm::vec4& color) = 0;
		virtual void setViewPort(uint32_t x, uint32_t y, uint32_t width, uint32_t height) = 0;
		virtual void clear() = 0;

		virtual void drawIndexed(const Ref<VertexArray>& vertexArray,uint32_t count=0 ) = 0;
		virtual void drawLines(const Ref<VertexArray>& vertexArray, uint32_t vertexCount) =0;
		inline static API getAPI(){ return s_api; };

	private:
		static API s_api;
	};
}


