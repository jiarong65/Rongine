#include "Rongpch.h"
#include "Renderer3D.h"
#include "Rongine/Renderer/VertexArray.h"
#include "Rongine/Renderer/Shader.h"
#include "Rongine/Renderer/RenderCommand.h"
#include <glm/gtc/matrix_transform.hpp>
#include <array>

namespace Rongine {

	// 1. 定义 3D 顶点结构
	struct CubeVertex
	{
		glm::vec3 Position;
		glm::vec4 Color;
		glm::vec2 TexCoord;
		float TexIndex;
		float TilingFactor;
		// 如果需要光照，以后在这里加 glm::vec3 Normal;
	};

	// 2. 渲染器数据结构
	struct Renderer3DData
	{
		static const uint32_t MaxCubes = 10000;             // 最大支持 10000 个立方体一次提交
		static const uint32_t MaxVertices = MaxCubes * 8;   // 每个立方体 8 个顶点
		static const uint32_t MaxIndices = MaxCubes * 36;   // 每个立方体 6个面 * 2个三角形 * 3个点 = 36 索引
		static const uint32_t MaxTextureSlots = 32;

		Ref<VertexArray> CubeVA;
		Ref<VertexBuffer> CubeVB;
		Ref<Shader> TextureShader; // 复用支持纹理的 Shader
		Ref<Texture2D> WhiteTexture;

		uint32_t CubeIndexCount = 0;
		CubeVertex* CubeVertexBufferBase = nullptr;
		CubeVertex* CubeVertexBufferPtr = nullptr;

		std::array<Ref<Texture2D>, MaxTextureSlots> TextureSlots;
		uint32_t TextureSlotIndex = 1; // 0留给白纹理

		glm::vec4 CubeVertexPositions[8]; // 标准立方体的8个顶点位置缓存

		Renderer3D::Statistics Stats;
	};

	static Renderer3DData s_Data;

	void Renderer3D::init()
	{
		s_Data.CubeVA = VertexArray::create();

		// 创建动态顶点缓冲
		s_Data.CubeVB = VertexBuffer::create(s_Data.MaxVertices * sizeof(CubeVertex));
		s_Data.CubeVB->setLayout({
			{ ShaderDataType::Float3, "a_Position" },
			{ ShaderDataType::Float4, "a_Color" },
			{ ShaderDataType::Float2, "a_TexCoord" },
			{ ShaderDataType::Float,  "a_TexIndex" },
			{ ShaderDataType::Float,  "a_TilingFactor" }
			});
		s_Data.CubeVA->addVertexBuffer(s_Data.CubeVB);

		// 分配 CPU 内存
		s_Data.CubeVertexBufferBase = new CubeVertex[s_Data.MaxVertices];

		// --- 预计算索引缓冲 (Batch Index Buffer) ---
		// 我们需要填满 MaxIndices 这么多索引
		uint32_t* cubeIndices = new uint32_t[s_Data.MaxIndices];
		uint32_t offset = 0;
		for (uint32_t i = 0; i < s_Data.MaxIndices; i += 36)
		{
			// 这种索引顺序对应 8 顶点的立方体
			// 前面
			cubeIndices[i + 0] = offset + 0; cubeIndices[i + 1] = offset + 1; cubeIndices[i + 2] = offset + 2;
			cubeIndices[i + 3] = offset + 2; cubeIndices[i + 4] = offset + 3; cubeIndices[i + 5] = offset + 0;
			// 右面
			cubeIndices[i + 6] = offset + 1; cubeIndices[i + 7] = offset + 5; cubeIndices[i + 8] = offset + 6;
			cubeIndices[i + 9] = offset + 6; cubeIndices[i + 10] = offset + 2; cubeIndices[i + 11] = offset + 1;
			// 后面
			cubeIndices[i + 12] = offset + 7; cubeIndices[i + 13] = offset + 6; cubeIndices[i + 14] = offset + 5;
			cubeIndices[i + 15] = offset + 5; cubeIndices[i + 16] = offset + 4; cubeIndices[i + 17] = offset + 7;
			// 左面
			cubeIndices[i + 18] = offset + 4; cubeIndices[i + 19] = offset + 0; cubeIndices[i + 20] = offset + 3;
			cubeIndices[i + 21] = offset + 3; cubeIndices[i + 22] = offset + 7; cubeIndices[i + 23] = offset + 4;
			// 底面
			cubeIndices[i + 24] = offset + 4; cubeIndices[i + 25] = offset + 5; cubeIndices[i + 26] = offset + 1;
			cubeIndices[i + 27] = offset + 1; cubeIndices[i + 28] = offset + 0; cubeIndices[i + 29] = offset + 4;
			// 顶面
			cubeIndices[i + 30] = offset + 3; cubeIndices[i + 31] = offset + 2; cubeIndices[i + 32] = offset + 6;
			cubeIndices[i + 33] = offset + 6; cubeIndices[i + 34] = offset + 7; cubeIndices[i + 35] = offset + 3;

			offset += 8; // 下一个立方体的索引从 +8 开始
		}

		Ref<IndexBuffer> cubeIB = IndexBuffer::create(cubeIndices, s_Data.MaxIndices);
		s_Data.CubeVA->setIndexBuffer(cubeIB);
		delete[] cubeIndices;

		// --- 初始化纹理支持 ---
		s_Data.WhiteTexture = Texture2D::create(1, 1);
		uint32_t whiteTextureData = 0xffffffff;
		s_Data.WhiteTexture->setData(&whiteTextureData, sizeof(uint32_t));

		int32_t samplers[s_Data.MaxTextureSlots];
		for (uint32_t i = 0; i < s_Data.MaxTextureSlots; i++) samplers[i] = i;

		// 注意：这里需要一个支持 Texture 的 3D Shader，你可以复用 Renderer2D 的 Shader
		// 或者创建一个新的 "Texture3D.glsl"
		s_Data.TextureShader = Shader::create("assets/shaders/Texture.glsl");
		s_Data.TextureShader->bind();
		s_Data.TextureShader->setIntArray("u_Textures", samplers, s_Data.MaxTextureSlots);

		s_Data.TextureSlots[0] = s_Data.WhiteTexture;

		// --- 缓存标准立方体位置 ---
		s_Data.CubeVertexPositions[0] = { -0.5f, -0.5f,  0.5f, 1.0f };
		s_Data.CubeVertexPositions[1] = { 0.5f, -0.5f,  0.5f, 1.0f };
		s_Data.CubeVertexPositions[2] = { 0.5f,  0.5f,  0.5f, 1.0f };
		s_Data.CubeVertexPositions[3] = { -0.5f,  0.5f,  0.5f, 1.0f };
		s_Data.CubeVertexPositions[4] = { -0.5f, -0.5f, -0.5f, 1.0f };
		s_Data.CubeVertexPositions[5] = { 0.5f, -0.5f, -0.5f, 1.0f };
		s_Data.CubeVertexPositions[6] = { 0.5f,  0.5f, -0.5f, 1.0f };
		s_Data.CubeVertexPositions[7] = { -0.5f,  0.5f, -0.5f, 1.0f };
	}

	void Renderer3D::shutdown()
	{
		delete[] s_Data.CubeVertexBufferBase;
	}

	void Renderer3D::beginScene(const PerspectiveCamera& camera)
	{
		s_Data.TextureShader->bind();
		s_Data.TextureShader->setMat4("u_ViewProjection", camera.getViewProjectionMatrix());

		s_Data.CubeIndexCount = 0;
		s_Data.CubeVertexBufferPtr = s_Data.CubeVertexBufferBase;
		s_Data.TextureSlotIndex = 1;
	}

	void Renderer3D::endScene()
	{
		uint32_t dataSize = (uint32_t)((uint8_t*)s_Data.CubeVertexBufferPtr - (uint8_t*)s_Data.CubeVertexBufferBase);
		s_Data.CubeVB->setData(s_Data.CubeVertexBufferBase, dataSize);
		flush();
	}

	void Renderer3D::flush()
	{
		if (s_Data.CubeIndexCount == 0) return;

		// 绑定所有纹理
		for (uint32_t i = 0; i < s_Data.TextureSlotIndex; i++)
			s_Data.TextureSlots[i]->bind(i);

		s_Data.CubeVA->bind();
		RenderCommand::drawIndexed(s_Data.CubeVA, s_Data.CubeIndexCount);
		s_Data.Stats.DrawCalls++;
	}

	void Renderer3D::flushAndReset()
	{
		endScene();
		s_Data.CubeIndexCount = 0;
		s_Data.CubeVertexBufferPtr = s_Data.CubeVertexBufferBase;
		s_Data.TextureSlotIndex = 1;
	}

	void Renderer3D::drawCube(const glm::vec3& position, const glm::vec3& size, const glm::vec4& color)
	{
		// 纯色绘制，使用 WhiteTexture
		drawCube(position, size, s_Data.WhiteTexture, color);
	}

	void Renderer3D::drawCube(const glm::vec3& position, const glm::vec3& size, const Ref<Texture2D>& texture, const glm::vec4& tintColor)
	{
		// 1. 如果 Buffer 满了，先提交一次
		if (s_Data.CubeIndexCount >= Renderer3DData::MaxIndices)
			flushAndReset();

		// 2. 查找或添加纹理
		float textureIndex = 0.0f;
		for (uint32_t i = 1; i < s_Data.TextureSlotIndex; i++)
		{
			if (*s_Data.TextureSlots[i].get() == *texture.get())
			{
				textureIndex = (float)i;
				break;
			}
		}

		if (textureIndex == 0.0f)
		{
			if (s_Data.TextureSlotIndex >= Renderer3DData::MaxTextureSlots)
				flushAndReset();

			textureIndex = (float)s_Data.TextureSlotIndex;
			s_Data.TextureSlots[s_Data.TextureSlotIndex] = texture;
			s_Data.TextureSlotIndex++;
		}

		// 3. 计算变换矩阵 (Model Matrix) - 在 CPU 端完成
		glm::mat4 transform = glm::translate(glm::mat4(1.0f), position) * glm::scale(glm::mat4(1.0f), size);

		// 4. 填充 8 个顶点数据
		for (int i = 0; i < 8; i++)
		{
			// A. 计算世界坐标：原始位置 * 变换矩阵
			s_Data.CubeVertexBufferPtr->Position = transform * s_Data.CubeVertexPositions[i];

			// B. 颜色
			s_Data.CubeVertexBufferPtr->Color = tintColor;

			// C. 设置 UV 坐标 (核心修改！)
			// 将局部坐标 (-0.5 ~ 0.5) 映射到 UV (0.0 ~ 1.0)
			// 这样每个面虽然会有拉伸，但至少能看到纹理图案，而不是纯色
			s_Data.CubeVertexBufferPtr->TexCoord = {
				s_Data.CubeVertexPositions[i].x + 0.5f,
				s_Data.CubeVertexPositions[i].y + 0.5f
			};

			// D. 纹理索引
			s_Data.CubeVertexBufferPtr->TexIndex = textureIndex;

			// E. 平铺系数
			s_Data.CubeVertexBufferPtr->TilingFactor = 1.0f;

			// 指针后移
			s_Data.CubeVertexBufferPtr++;
		}

		s_Data.CubeIndexCount += 36; // 增加索引计数 (12个三角形 * 3)
		s_Data.Stats.CubeCount++;
	}

	Renderer3D::Statistics Renderer3D::getStatistics() { return s_Data.Stats; }
	void Renderer3D::resetStatistics() { memset(&s_Data.Stats, 0, sizeof(Statistics)); }
}