#include "Rongpch.h"
#include "BVH.h"
#include "Rongine/Core/Log.h"

namespace Rongine {

    BVHBuilder::BVHBuilder(const std::vector<BVHTriangle>& triangles)
    {
        m_BuildTriangles = triangles;
        m_Nodes.reserve(triangles.size() * 2);
        m_SortedIndices.reserve(triangles.size());

        // 1. 创建根节点 (Index 0)
        GPUBVHNode root;
        root.LeftChildIndex = 0;
        root.RightChildIndex = 0;
        root.MinX = 0.0f; root.MinY = 0.0f; root.MinZ = 0.0f;
        root.MaxX = 0.0f; root.MaxY = 0.0f; root.MaxZ = 0.0f;
        m_Nodes.push_back(root);

        // 2. 开始递归构建
        SplitBVHNode(0, 0, (int)m_BuildTriangles.size(), 0);

        // 3. 生成最终索引表
        // 因为构建过程中 m_BuildTriangles 被 std::partition 重排了
        // 我们需要把重排后的“原始ID”记录下来传给 GPU
        for (const auto& tri : m_BuildTriangles) {
            m_SortedIndices.push_back(tri.Index);
        }

        RONG_CORE_INFO("BVH Built Successfully: {0} Triangles -> {1} Nodes", triangles.size(), m_Nodes.size());
    }

    void BVHBuilder::UpdateNodeBounds(int nodeIndex, int start, int end)
    {
        glm::vec3 min(1e30f);
        glm::vec3 max(-1e30f);

        for (int i = start; i < end; i++) {
            const auto& tri = m_BuildTriangles[i];

            min = glm::min(min, tri.V0);
            min = glm::min(min, tri.V1);
            min = glm::min(min, tri.V2);

            max = glm::max(max, tri.V0);
            max = glm::max(max, tri.V1);
            max = glm::max(max, tri.V2);
        }

        m_Nodes[nodeIndex].MinX = min.x;
        m_Nodes[nodeIndex].MinY = min.y;
        m_Nodes[nodeIndex].MinZ = min.z;

        m_Nodes[nodeIndex].MaxX = max.x;
        m_Nodes[nodeIndex].MaxY = max.y;
        m_Nodes[nodeIndex].MaxZ = max.z;
    }

    void BVHBuilder::SplitBVHNode(int nodeIndex, int start, int end, int depth)
    {
        int count = end - start;

        // 1. 先计算当前节点的包围盒
        UpdateNodeBounds(nodeIndex, start, end);
        GPUBVHNode& node = m_Nodes[nodeIndex]; // 获取引用

        // 2. 终止条件：如果是叶子节点 (三角形很少，或者深度太深)
        if (count <= 4 || depth > 32)
        {
            // === 标记为叶子节点 ===
            // 你的注释：LeftChildIndex < 0 表示叶子
            // 我们存储 -(Start + 1) 以避免 0 的歧义
            node.LeftChildIndex = -(float)(start + 1);

            // 你的注释：叶子节点时存储 TriangleCount
            node.RightChildIndex = (float)count;
            return;
        }

        // 3. 寻找最长轴用于切割
        glm::vec3 boxMin(node.MinX, node.MinY, node.MinZ);
        glm::vec3 boxMax(node.MaxX, node.MaxY, node.MaxZ);

        glm::vec3 extent = boxMax - boxMin;
        int axis = 0;
        if (extent.y > extent.x) axis = 1;
        if (extent.z > extent[axis]) axis = 2;

        float splitPos = (boxMin[axis] + boxMax[axis]) * 0.5f; // 中点分割
        // 或者使用质心包围盒的中点 (更稳健)：
        // float splitPos = 0.0f; ... (略，先用简单的空间中点)

        // 4. 执行划分 (Partition)
        // 将三角形数组分为两部分：左边 < splitPos，右边 >= splitPos
        int mid = start;
        auto it = std::partition(m_BuildTriangles.begin() + start, m_BuildTriangles.begin() + end,
            [axis, splitPos](const BVHTriangle& tri) {
                return tri.Centroid[axis] < splitPos;
            });
        mid = (int)(it - m_BuildTriangles.begin());

        // 5. 兜底策略：如果切分失败 (比如所有三角形重心都在一边)，强制对半切
        if (mid == start || mid == end) {
            mid = start + (count / 2);
            std::nth_element(m_BuildTriangles.begin() + start,
                m_BuildTriangles.begin() + mid,
                m_BuildTriangles.begin() + end,
                [axis](const BVHTriangle& a, const BVHTriangle& b) {
                    return a.Centroid[axis] < b.Centroid[axis];
                });
        }

        // 6. 创建子节点
        // 注意：m_Nodes.push_back 可能会导致 vector 扩容，从而使上面的 `node` 引用失效！
        // 所以这里我们要先记录索引，push 之后再重新获取 node 的访问权
        int leftChildIdx = (int)m_Nodes.size();
        m_Nodes.push_back(GPUBVHNode()); // Left
        m_Nodes.push_back(GPUBVHNode()); // Right
        int rightChildIdx = leftChildIdx + 1;

        // 重新获取当前节点 (防止扩容引用失效)
        m_Nodes[nodeIndex].LeftChildIndex = (float)leftChildIdx;
        m_Nodes[nodeIndex].RightChildIndex = (float)rightChildIdx;

        // 7. 递归构建子节点
        SplitBVHNode(leftChildIdx, start, mid, depth + 1);
        SplitBVHNode(rightChildIdx, mid, end, depth + 1);
    }
}