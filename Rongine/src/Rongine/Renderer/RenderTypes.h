#pragma once
#include <glm/glm.hpp>

namespace Rongine {

	struct CubeVertex
	{
		glm::vec3 Position;
		glm::vec3 Normal;   // 法线
		glm::vec4 Color;
		glm::vec2 TexCoord;
		float TexIndex;
		float TilingFactor;
		int FaceID;
	};

	struct BatchLineVertex
	{
		glm::vec3 Position;
		glm::vec4 Color;
	};

	//SSBO
	struct GPUVertex
	{
		glm::vec3 Position;
		float _pad1;       // 填充位，保证 16 字节对齐
		glm::vec3 Normal;
		float _pad2;
		glm::vec2 TexCoord;
		glm::vec2 _pad3;   // 填充到 16 字节
	};

	//struct GPUMaterial
	//{
	//	glm::vec3 Albedo;  
	//	float Roughness;   

	//	float Metallic;    
	//	float Emission;    

	//	int SpectralIndex = -1; //默认为 -1 表示使用 RGB
	//	float _padding = 0.0f;  
	//};

	struct GPUMaterial {
		glm::vec4 AlbedoRoughness; // rgb=BaseColor, a=Roughness
		float Metallic;
		float Emission;

		int SpectralIndex0; // 对应 Slot0 (n / Reflectance)
		int SpectralIndex1; // 对应 Slot1 (k / IOR)

		int Type;           // 0=Diffuse, 1=Conductor, 2=Dielectric
		float _pad1;        // 填充位 1
		float _pad2;        // 填充位 2
		float _pad3;        // 填充位 3 (凑齐 48 bytes 或 64 bytes)
	};

	struct TriangleData
	{
		uint32_t v0, v1, v2; // 顶点的索引
		uint32_t MaterialID; // 材质索引
	};

	//AABB
	struct AABB {
		glm::vec3 Min = glm::vec3(1e30f); // 初始化为无穷大，方便 Grow
		glm::vec3 Max = glm::vec3(-1e30f);

		AABB() = default;
		AABB(const glm::vec3& min, const glm::vec3& max) : Min(min), Max(max) {}

		glm::vec3 GetCenter() const { return (Min + Max) * 0.5f; }
		glm::vec3 GetSize() const { return Max - Min; }

		// 扩展包围盒以包含一个点
		void Grow(const glm::vec3& p) {
			Min = glm::min(Min, p);
			Max = glm::max(Max, p);
		}

		// 扩展包围盒以包含另一个包围盒
		void Grow(const AABB& b) {
			// 简单的有效性检查
			if (b.Min.x > b.Max.x) return;
			Min = glm::min(Min, b.Min);
			Max = glm::max(Max, b.Max);
		}

		// 计算表面积 (用于 SAH，即使暂时不用也可以留着)
		float Area() const {
			glm::vec3 d = Max - Min;
			if (d.x < 0 || d.y < 0 || d.z < 0) return 0.0f;
			return 2.0f * (d.x * d.y + d.y * d.z + d.z * d.x);
		}
	};
}