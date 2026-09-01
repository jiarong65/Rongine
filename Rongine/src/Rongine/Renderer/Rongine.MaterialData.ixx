module;
#include <glm/glm.hpp>

export module Rongine.MaterialData;

export namespace Rongine {

	struct MaterialData
	{
		glm::vec3 Albedo = { 1.0f, 1.0f, 1.0f };
		float Roughness = 0.5f;

		float Metallic = 0.0f;
		float Emission = 0.0f;
		float _pad0 = 0.0f;
		float _pad1 = 0.0f;
	};
}
