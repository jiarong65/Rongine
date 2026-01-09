#pragma once

#include <glm/glm.hpp>

namespace Rongine {

    class CADBoolean
    {
    public:
        enum class Operation { Cut, Fuse, Common };

        // 核心函数：对两个 Shape 进行布尔运算
        // shapeA: 主物体 (被挖的)
        // transformA: 主物体的世界变换矩阵
        // shapeB: 工具物体 (用来挖人的)
        // transformB: 工具物体的世界变换矩阵
        // 返回值: void* (新的 TopoDS_Shape 指针)
        static void* Perform(void* shapeA, const glm::mat4& transformA,
            void* shapeB, const glm::mat4& transformB,
            Operation op);
    };
}