module;
#include <glm/glm.hpp>
#include <cmath>

export module Rongine.Math;

export namespace Rongine::Math {
    bool RayPlaneIntersection(const glm::vec3& rayOrigin, const glm::vec3& rayDir,
        const glm::vec3& planeOrigin, const glm::vec3& planeNormal,
        glm::vec3& outIntersection)
    {
        float denom = glm::dot(planeNormal, rayDir);

        if (std::abs(denom) > 1e-6f)
        {
            glm::vec3 p0l0 = planeOrigin - rayOrigin;
            float t = glm::dot(p0l0, planeNormal) / denom;

            if (t >= 0.0f)
            {
                outIntersection = rayOrigin + rayDir * t;
                return true;
            }
        }
        return false;
    }
}
