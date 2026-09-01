module;
#include <vector>
#include <cstdio>
#include <algorithm>
#include <glm/glm.hpp>

export module Rongine.BVH;

export import Rongine.RendererAcceleration;
export import Rongine.RendererTypes;

export namespace Rongine {

	class BVHBuilder {
	public:
		BVHBuilder(const std::vector<BVHTriangle>& triangles);

		const std::vector<GPUBVHNode>& GetNodes() const { return m_Nodes; }
		const std::vector<uint32_t>& GetSortedIndices() const { return m_SortedIndices; }

	private:
		void SplitBVHNode(int nodeIndex, int start, int end, int depth);
		void UpdateNodeBounds(int nodeIndex, int start, int end);

		std::vector<GPUBVHNode> m_Nodes;
		std::vector<BVHTriangle> m_BuildTriangles;
		std::vector<uint32_t> m_SortedIndices;
	};

	BVHBuilder::BVHBuilder(const std::vector<BVHTriangle>& triangles)
	{
		m_BuildTriangles = triangles;
		m_Nodes.reserve(triangles.size() * 2);
		m_SortedIndices.reserve(triangles.size());

		GPUBVHNode root;
		root.LeftChildIndex = 0;
		root.RightChildIndex = 0;
		root.MinX = 0.0f; root.MinY = 0.0f; root.MinZ = 0.0f;
		root.MaxX = 0.0f; root.MaxY = 0.0f; root.MaxZ = 0.0f;
		m_Nodes.push_back(root);

		SplitBVHNode(0, 0, (int)m_BuildTriangles.size(), 0);

		for (const auto& tri : m_BuildTriangles) {
			m_SortedIndices.push_back(tri.Index);
		}

		std::printf("BVH Built Successfully: %zu Triangles -> %zu Nodes\n", triangles.size(), m_Nodes.size());
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

		UpdateNodeBounds(nodeIndex, start, end);
		GPUBVHNode& node = m_Nodes[nodeIndex];

		if (count <= 4 || depth > 32)
		{
			node.LeftChildIndex = -(float)(start + 1);
			node.RightChildIndex = (float)count;
			return;
		}

		glm::vec3 boxMin(node.MinX, node.MinY, node.MinZ);
		glm::vec3 boxMax(node.MaxX, node.MaxY, node.MaxZ);

		glm::vec3 extent = boxMax - boxMin;
		int axis = 0;
		if (extent.y > extent.x) axis = 1;
		if (extent.z > extent[axis]) axis = 2;

		float splitPos = (boxMin[axis] + boxMax[axis]) * 0.5f;

		int mid = start;
		auto it = std::partition(m_BuildTriangles.begin() + start, m_BuildTriangles.begin() + end,
			[axis, splitPos](const BVHTriangle& tri) {
				return tri.Centroid[axis] < splitPos;
			});
		mid = (int)(it - m_BuildTriangles.begin());

		if (mid == start || mid == end) {
			mid = start + (count / 2);
			std::nth_element(m_BuildTriangles.begin() + start,
				m_BuildTriangles.begin() + mid,
				m_BuildTriangles.begin() + end,
				[axis](const BVHTriangle& a, const BVHTriangle& b) {
					return a.Centroid[axis] < b.Centroid[axis];
				});
		}

		int leftChildIdx = (int)m_Nodes.size();
		m_Nodes.push_back(GPUBVHNode());
		m_Nodes.push_back(GPUBVHNode());
		int rightChildIdx = leftChildIdx + 1;

		m_Nodes[nodeIndex].LeftChildIndex = (float)leftChildIdx;
		m_Nodes[nodeIndex].RightChildIndex = (float)rightChildIdx;

		SplitBVHNode(leftChildIdx, start, mid, depth + 1);
		SplitBVHNode(rightChildIdx, mid, end, depth + 1);
	}
}
