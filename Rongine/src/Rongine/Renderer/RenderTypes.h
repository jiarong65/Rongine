#pragma once
#include <glm/glm.hpp>

namespace Rongine {

	struct CubeVertex
	{
		glm::vec3 Position;
		glm::vec3 Normal;   // 法线
		glm::vec4 Color;
		glm::vec2 TexCoord;
		float TexIndex;
		float TilingFactor;
		int FaceID;
	};

	struct BatchLineVertex
	{
		glm::vec3 Position;
		glm::vec4 Color;
	};
}