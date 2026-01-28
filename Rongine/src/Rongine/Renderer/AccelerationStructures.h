#pragma once
#include <glm/glm.hpp>
#include <vector>

namespace Rongine {

    enum class AccelType {
        None = 0,
        BVH = 1,
        Octree = 2
    };

    // --- BVH 节点 (32 bytes) ---
    //struct GPUBVHNode {
    //    glm::vec3 AABBMin;
    //    float LeftChildIndex; // < 0 表示叶子节点
    //    glm::vec3 AABBMax;
    //    float RightChildIndex; // 叶子节点时存储 TriangleCount
    //};

    struct GPUBVHNode {
        float MinX, MinY, MinZ; // 12 bytes
        float LeftChildIndex;   // 4 bytes

        float MaxX, MaxY, MaxZ; // 12 bytes
        float RightChildIndex;  // 4 bytes
    };

    // --- 八叉树节点 (通常比较大，简化版) ---
    // 线性八叉树 (Linear Octree) 结构
    struct GPUOctreeNode {
        glm::vec3 Center;
        float Size;           // 节点半长
        float Children[8];    // 8个子节点的索引 (-1 表示无)
        float TriangleStartIndex; //如果是叶子，指向索引列表的起始
        float TriangleCount;      //如果是叶子，包含的三角形数量
        float _pad[2]; // 填充至 16 字节对齐
    };

    struct BVHTriangle {
        glm::vec3 V0, V1, V2;
        glm::vec3 Centroid;
        uint32_t Index; // 原始索引 (用于 SSBO Binding 8)
    };
}