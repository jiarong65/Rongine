#pragma once

#include "Rongine/Renderer/PerspectiveCamera.h"
#include "Rongine/Renderer/Texture.h"
#include "Rongine/Renderer/Renderer2D.h" // 复用 2D 的一些统计结构

#include <glm/glm.hpp>

namespace Rongine {

	class Renderer3D
	{
	public:
		static void init();
		static void shutdown();

		static void beginScene(const PerspectiveCamera& camera);
		static void endScene();
		static void flush();

		// --- 基础绘制 ---
		static void drawCube(const glm::vec3& position, const glm::vec3& size, const glm::vec4& color);

		// --- 支持纹理的绘制 ---
		static void drawCube(const glm::vec3& position, const glm::vec3& size, const Ref<Texture2D>& texture, const glm::vec4& tintColor = glm::vec4(1.0f));

		// --- 统计信息 ---
		struct Statistics
		{
			uint32_t DrawCalls = 0;
			uint32_t CubeCount = 0;
			uint32_t GetTotalVertexCount() { return CubeCount * 8; }
			uint32_t GetTotalIndexCount() { return CubeCount * 36; }
		};

		static Statistics getStatistics();
		static void resetStatistics();

	private:
		static void flushAndReset();
	};
}