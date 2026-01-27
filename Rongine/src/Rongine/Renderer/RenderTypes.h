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

	struct GPUMaterial
	{
		glm::vec3 Albedo;  
		float Roughness;   

		float Metallic;    
		float Emission;    

		int SpectralIndex = -1; //默认为 -1 表示使用 RGB
		float _padding = 0.0f;  
	};

	struct TriangleData
	{
		uint32_t v0, v1, v2; // 顶点的索引
		uint32_t MaterialID; // 材质索引
	};
}