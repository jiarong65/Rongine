#pragma once
#include "Rongine/Renderer/VertexArray.h"
#include "Rongine/Renderer/Renderer3D.h" // 获取 CubeVertex 定义
#include <vector>
#include <cmath>

namespace Rongine {

    struct FaceInfo {
        glm::vec3 LocalCenter;
        glm::vec3 LocalNormal;
    };

    class GeometryUtils {
    public:
        // majorRadius: 大环半径, minorRadius: 管子半径
        static Ref<VertexArray> CreateTorus(float majorRadius, float minorRadius, int majorSegments, int minorSegments);

        static FaceInfo CalculateFaceCenter(const std::vector<CubeVertex>& vertices, int targetFaceID);
    };
}