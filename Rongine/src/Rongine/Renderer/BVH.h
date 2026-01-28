#pragma once
#include "Rongine/Renderer/AccelerationStructures.h"
#include "Rongine/Renderer/RenderTypes.h" 

#include <vector>
#include <glm/glm.hpp>

namespace Rongine {
    class BVHBuilder {
    public:
        BVHBuilder(const std::vector<BVHTriangle>& triangles);

        // 获取构建好的节点数组 
        const std::vector<GPUBVHNode>& GetNodes() const { return m_Nodes; }

        // 获取重排后的索引映射
        const std::vector<uint32_t>& GetSortedIndices() const { return m_SortedIndices; }

    private:
        // 递归构建函数
        void SplitBVHNode(int nodeIndex, int start, int end, int depth);

        // 更新节点的 AABB 包围盒
        void UpdateNodeBounds(int nodeIndex, int start, int end);

        std::vector<GPUBVHNode> m_Nodes;           // 最终输出给 GPU 的节点
        std::vector<BVHTriangle> m_BuildTriangles; // 构建时的临时三角形数据
        std::vector<uint32_t> m_SortedIndices;     // 最终输出的索引
    };

}