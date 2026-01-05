#include "Rongpch.h"
#include "Renderer3D.h"
#include "Rongine/Renderer/VertexArray.h"
#include "Rongine/Renderer/Shader.h"
#include "Rongine/Renderer/RenderCommand.h"
#include <glm/gtc/matrix_transform.hpp>
#include <array>

namespace Rongine {

	struct CubeVertex
	{
		glm::vec3 Position;
		glm::vec3 Normal;   // 法线
		glm::vec4 Color;
		glm::vec2 TexCoord;
		float TexIndex;
		float TilingFactor;
	};

	struct Renderer3DData
	{
		static const uint32_t MaxCubes = 10000;
		static const uint32_t MaxVertices = MaxCubes * 24;
		static const uint32_t MaxIndices = MaxCubes * 36;
		static const uint32_t MaxTextureSlots = 32;

		Ref<VertexArray> CubeVA;
		Ref<VertexBuffer> CubeVB;
		Ref<Shader> TextureShader;
		Ref<Texture2D> WhiteTexture;

		uint32_t CubeIndexCount = 0;
		CubeVertex* CubeVertexBufferBase = nullptr;
		CubeVertex* CubeVertexBufferPtr = nullptr;

		std::array<Ref<Texture2D>, MaxTextureSlots> TextureSlots;
		uint32_t TextureSlotIndex = 1;

		glm::vec4 CubeVertexPositions[24];
		glm::vec3 CubeVertexNormals[24];

		Renderer3D::Statistics Stats;
	};

	static Renderer3DData s_Data;

	void Renderer3D::init()
	{
		s_Data.CubeVA = VertexArray::create();

		s_Data.CubeVB = VertexBuffer::create(s_Data.MaxVertices * sizeof(CubeVertex));
		s_Data.CubeVB->setLayout({
			{ ShaderDataType::Float3, "a_Position" },
			{ ShaderDataType::Float3, "a_Normal" },
			{ ShaderDataType::Float4, "a_Color" },
			{ ShaderDataType::Float2, "a_TexCoord" },
			{ ShaderDataType::Float,  "a_TexIndex" },
			{ ShaderDataType::Float,  "a_TilingFactor" }
			});
		s_Data.CubeVA->addVertexBuffer(s_Data.CubeVB);

		s_Data.CubeVertexBufferBase = new CubeVertex[s_Data.MaxVertices];

		uint32_t* cubeIndices = new uint32_t[s_Data.MaxIndices];
		uint32_t offset = 0;
		for (uint32_t i = 0; i < s_Data.MaxIndices; i += 6)
		{
			cubeIndices[i + 0] = offset + 0;
			cubeIndices[i + 1] = offset + 1;
			cubeIndices[i + 2] = offset + 2;

			cubeIndices[i + 3] = offset + 2;
			cubeIndices[i + 4] = offset + 3;
			cubeIndices[i + 5] = offset + 0;

			offset += 4;
		}

		Ref<IndexBuffer> cubeIB = IndexBuffer::create(cubeIndices, s_Data.MaxIndices);
		s_Data.CubeVA->setIndexBuffer(cubeIB);
		delete[] cubeIndices;

		s_Data.WhiteTexture = Texture2D::create(1, 1);
		uint32_t whiteTextureData = 0xffffffff;
		s_Data.WhiteTexture->setData(&whiteTextureData, sizeof(uint32_t));

		int32_t samplers[s_Data.MaxTextureSlots];
		for (uint32_t i = 0; i < s_Data.MaxTextureSlots; i++) samplers[i] = i;

		s_Data.TextureShader = Shader::create("assets/shaders/Texture.glsl");
		s_Data.TextureShader->bind();
		s_Data.TextureShader->setIntArray("u_Textures", samplers, s_Data.MaxTextureSlots);

		s_Data.TextureSlots[0] = s_Data.WhiteTexture;

		// --- 初始化 24 个顶点的标准位置 ---
		// Front
		s_Data.CubeVertexPositions[0] = { -0.5f, -0.5f,  0.5f, 1.0f };
		s_Data.CubeVertexPositions[1] = { 0.5f, -0.5f,  0.5f, 1.0f };
		s_Data.CubeVertexPositions[2] = { 0.5f,  0.5f,  0.5f, 1.0f };
		s_Data.CubeVertexPositions[3] = { -0.5f,  0.5f,  0.5f, 1.0f };
		// Right
		s_Data.CubeVertexPositions[4] = { 0.5f, -0.5f,  0.5f, 1.0f };
		s_Data.CubeVertexPositions[5] = { 0.5f, -0.5f, -0.5f, 1.0f };
		s_Data.CubeVertexPositions[6] = { 0.5f,  0.5f, -0.5f, 1.0f };
		s_Data.CubeVertexPositions[7] = { 0.5f,  0.5f,  0.5f, 1.0f };
		// Back
		s_Data.CubeVertexPositions[8] = { 0.5f, -0.5f, -0.5f, 1.0f };
		s_Data.CubeVertexPositions[9] = { -0.5f, -0.5f, -0.5f, 1.0f };
		s_Data.CubeVertexPositions[10] = { -0.5f,  0.5f, -0.5f, 1.0f };
		s_Data.CubeVertexPositions[11] = { 0.5f,  0.5f, -0.5f, 1.0f };
		// Left
		s_Data.CubeVertexPositions[12] = { -0.5f, -0.5f, -0.5f, 1.0f };
		s_Data.CubeVertexPositions[13] = { -0.5f, -0.5f,  0.5f, 1.0f };
		s_Data.CubeVertexPositions[14] = { -0.5f,  0.5f,  0.5f, 1.0f };
		s_Data.CubeVertexPositions[15] = { -0.5f,  0.5f, -0.5f, 1.0f };
		// Top
		s_Data.CubeVertexPositions[16] = { -0.5f,  0.5f,  0.5f, 1.0f };
		s_Data.CubeVertexPositions[17] = { 0.5f,  0.5f,  0.5f, 1.0f };
		s_Data.CubeVertexPositions[18] = { 0.5f,  0.5f, -0.5f, 1.0f };
		s_Data.CubeVertexPositions[19] = { -0.5f,  0.5f, -0.5f, 1.0f };
		// Bottom
		s_Data.CubeVertexPositions[20] = { -0.5f, -0.5f, -0.5f, 1.0f };
		s_Data.CubeVertexPositions[21] = { 0.5f, -0.5f, -0.5f, 1.0f };
		s_Data.CubeVertexPositions[22] = { 0.5f, -0.5f,  0.5f, 1.0f };
		s_Data.CubeVertexPositions[23] = { -0.5f, -0.5f,  0.5f, 1.0f };

		// --- 初始化法线 ---
		for (int i = 0; i < 4; i++) s_Data.CubeVertexNormals[i] = { 0.0f, 0.0f, 1.0f }; // Front
		for (int i = 4; i < 8; i++) s_Data.CubeVertexNormals[i] = { 1.0f, 0.0f, 0.0f }; // Right
		for (int i = 8; i < 12; i++) s_Data.CubeVertexNormals[i] = { 0.0f, 0.0f, -1.0f };// Back
		for (int i = 12; i < 16; i++) s_Data.CubeVertexNormals[i] = { -1.0f, 0.0f, 0.0f };// Left
		for (int i = 16; i < 20; i++) s_Data.CubeVertexNormals[i] = { 0.0f, 1.0f, 0.0f }; // Top
		for (int i = 20; i < 24; i++) s_Data.CubeVertexNormals[i] = { 0.0f, -1.0f, 0.0f };// Bottom
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

	// --- 基础绘制 (Axis Aligned) ---
	void Renderer3D::drawCube(const glm::vec3& position, const glm::vec3& size, const glm::vec4& color)
	{
		drawCube(position, size, s_Data.WhiteTexture, color);
	}

	void Renderer3D::drawCube(const glm::vec3& position, const glm::vec3& size, const Ref<Texture2D>& texture, const glm::vec4& tintColor)
	{
		// 复用旋转绘制，旋转为 0 即可
		drawRotatedCube(position, size, 0.0f, { 0.0f, 1.0f, 0.0f }, texture, tintColor);
	}

	// --- 旋转绘制 ---
	void Renderer3D::drawRotatedCube(const glm::vec3& position, const glm::vec3& size, float rotation, const glm::vec3& axis, const glm::vec4& color)
	{
		drawRotatedCube(position, size, rotation, axis, s_Data.WhiteTexture, color);
	}

	void Renderer3D::drawRotatedCube(const glm::vec3& position, const glm::vec3& size, float rotation, const glm::vec3& axis, const Ref<Texture2D>& texture, const glm::vec4& tintColor)
	{
		if (s_Data.CubeIndexCount >= Renderer3DData::MaxIndices)
			flushAndReset();

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

		// 计算变换矩阵 (包含旋转)
		glm::mat4 rotationMat;
		if (rotation != 0.0f)
			rotationMat = glm::rotate(glm::mat4(1.0f), rotation, axis);
		else
			rotationMat = glm::mat4(1.0f);

		glm::mat4 transform = glm::translate(glm::mat4(1.0f), position)
			* rotationMat
			* glm::scale(glm::mat4(1.0f), size);

		// 计算法线矩阵 (只旋转)
		glm::mat3 normalMatrix = glm::mat3(rotationMat);

		// 填充 24 个顶点
		for (int i = 0; i < 24; i++)
		{
			s_Data.CubeVertexBufferPtr->Position = transform * s_Data.CubeVertexPositions[i];
			s_Data.CubeVertexBufferPtr->Normal = normalMatrix * s_Data.CubeVertexNormals[i]; // 旋转法线
			s_Data.CubeVertexBufferPtr->Color = tintColor;

			switch (i % 4)
			{
			case 0: s_Data.CubeVertexBufferPtr->TexCoord = { 0.0f, 0.0f }; break;
			case 1: s_Data.CubeVertexBufferPtr->TexCoord = { 1.0f, 0.0f }; break;
			case 2: s_Data.CubeVertexBufferPtr->TexCoord = { 1.0f, 1.0f }; break;
			case 3: s_Data.CubeVertexBufferPtr->TexCoord = { 0.0f, 1.0f }; break;
			}

			s_Data.CubeVertexBufferPtr->TexIndex = textureIndex;
			s_Data.CubeVertexBufferPtr->TilingFactor = 1.0f;
			s_Data.CubeVertexBufferPtr++;
		}

		s_Data.CubeIndexCount += 36;
		s_Data.Stats.CubeCount++;
	}

	Renderer3D::Statistics Renderer3D::getStatistics() { return s_Data.Stats; }
	void Renderer3D::resetStatistics() { memset(&s_Data.Stats, 0, sizeof(Statistics)); }
}