#include "Rongpch.h"
#include "Renderer3D.h"
#include "RenderCommand.h" // 确保包含 RenderCommand
#include <glm/gtc/matrix_transform.hpp>
#include <array>

namespace Rongine {

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

		// 【新增】初始化 Model 矩阵为单位矩阵，防止第一帧 Batch 渲染出错
		s_Data.TextureShader->setMat4("u_Model", glm::mat4(1.0f));

		// --- 初始化 24 个顶点的标准位置 ---
		// Front Face
		s_Data.CubeVertexPositions[0] = { -0.5f, -0.5f,  0.5f, 1.0f };
		s_Data.CubeVertexPositions[1] = { 0.5f, -0.5f,  0.5f, 1.0f };
		s_Data.CubeVertexPositions[2] = { 0.5f,  0.5f,  0.5f, 1.0f };
		s_Data.CubeVertexPositions[3] = { -0.5f,  0.5f,  0.5f, 1.0f };

		// Right Face
		s_Data.CubeVertexPositions[4] = { 0.5f, -0.5f,  0.5f, 1.0f };
		s_Data.CubeVertexPositions[5] = { 0.5f, -0.5f, -0.5f, 1.0f };
		s_Data.CubeVertexPositions[6] = { 0.5f,  0.5f, -0.5f, 1.0f };
		s_Data.CubeVertexPositions[7] = { 0.5f,  0.5f,  0.5f, 1.0f };

		// Back Face
		s_Data.CubeVertexPositions[8] = { 0.5f, -0.5f, -0.5f, 1.0f };
		s_Data.CubeVertexPositions[9] = { -0.5f, -0.5f, -0.5f, 1.0f };
		s_Data.CubeVertexPositions[10] = { -0.5f,  0.5f, -0.5f, 1.0f };
		s_Data.CubeVertexPositions[11] = { 0.5f,  0.5f, -0.5f, 1.0f };

		// Left Face
		s_Data.CubeVertexPositions[12] = { -0.5f, -0.5f, -0.5f, 1.0f };
		s_Data.CubeVertexPositions[13] = { -0.5f, -0.5f,  0.5f, 1.0f };
		s_Data.CubeVertexPositions[14] = { -0.5f,  0.5f,  0.5f, 1.0f };
		s_Data.CubeVertexPositions[15] = { -0.5f,  0.5f, -0.5f, 1.0f };

		// Top Face
		s_Data.CubeVertexPositions[16] = { -0.5f,  0.5f,  0.5f, 1.0f };
		s_Data.CubeVertexPositions[17] = { 0.5f,  0.5f,  0.5f, 1.0f };
		s_Data.CubeVertexPositions[18] = { 0.5f,  0.5f, -0.5f, 1.0f };
		s_Data.CubeVertexPositions[19] = { -0.5f,  0.5f, -0.5f, 1.0f };

		// Bottom Face
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

		// 1. 设置 ViewProjection (纯相机矩阵)
		s_Data.ViewProjection = camera.getViewProjectionMatrix();
		s_Data.TextureShader->setMat4("u_ViewProjection", s_Data.ViewProjection);

		// 2. 设置 ViewPos (用于高光)
		s_Data.TextureShader->setFloat3("u_ViewPos", camera.getPosition());

		// 3. 重置 u_Model 为单位矩阵
		// 确保接下来的 Batch 渲染 (drawCube) 不会继承上次的物体变换
		s_Data.TextureShader->setMat4("u_Model", glm::mat4(1.0f));

		//清空上一轮鼠标拾取缓存
		s_Data.TextureShader->setInt("u_EntityID", -1);

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

		// 【重要】Batch 渲染时，顶点已经在 CPU 变换过了，所以 GPU 的 u_Model 必须是 Identity
		s_Data.TextureShader->setMat4("u_Model", glm::mat4(1.0f));

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

	// ===========================================
	// 【核心修改】 Mesh Rendering
	// ===========================================
	void Renderer3D::drawModel(const Ref<VertexArray>& va, const glm::mat4& transform,int entityID)
	{
		// 1. 如果批处理里有方块没画，先画掉 (flush 会使用 Identity Model 矩阵)
		flush();

		s_Data.TextureShader->bind();

		// 2. 分别设置矩阵
		// u_Model: 物体的变换 (Shader 用它算 v_Position 和 v_Normal)
		s_Data.TextureShader->setMat4("u_Model", transform);

		// u_ViewProjection: 相机的 VP (保持 beginScene 设的值，不需要乘 transform)
		s_Data.TextureShader->setMat4("u_ViewProjection", s_Data.ViewProjection);

		s_Data.WhiteTexture->bind(0);

		s_Data.TextureShader->setInt("u_EntityID", entityID);

		va->bind();

		// 3. 【关键修复】显式传递 Index Count！
		// 之前这里可能默认为 0，导致什么都画不出来
		uint32_t count = va->getIndexBuffer()->getCount();
		RenderCommand::drawIndexed(va, count);

		s_Data.Stats.DrawCalls++;

		// 4. 画完后，恢复 u_Model 为单位矩阵
		// 否则之后如果再调用 drawCube，方块会飞到错误的地方
		s_Data.TextureShader->setMat4("u_Model", glm::mat4(1.0f));
	}

	Renderer3D::Statistics Renderer3D::getStatistics() { return s_Data.Stats; }
	void Renderer3D::resetStatistics() { memset(&s_Data.Stats, 0, sizeof(Statistics)); }
}

