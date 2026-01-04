#pragma once

#include "Rongine/Renderer/PerspectiveCamera.h"
#include "Rongine/Renderer/Texture.h"
#include <glm/glm.hpp>

namespace Rongine {

	class Renderer3D
	{
	public:
		static void init();
		static void shutdown();

		static void beginScene(const PerspectiveCamera& camera);
		static void endScene();

		// 绘制指定位置、大小、颜色的立方体
		static void drawCube(const glm::vec3& position, const glm::vec3& size, const glm::vec4& color);

	private:
		struct Renderer3DData;
	};
}