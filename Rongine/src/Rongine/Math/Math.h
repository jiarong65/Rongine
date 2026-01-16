#pragma once
#include <glm/glm.hpp>

namespace Rongine::Math {
    bool RayPlaneIntersection(const glm::vec3& rayOrigin, const glm::vec3& rayDir,
        const glm::vec3& planeOrigin, const glm::vec3& planeNormal,
        glm::vec3& outIntersection);

}


