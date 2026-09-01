module;
#include <glm/glm.hpp>

export module Rongine.GeometryData;

export namespace Rongine {

	struct FaceInfo {
		glm::vec3 LocalCenter;
		glm::vec3 LocalNormal;
	};
}
