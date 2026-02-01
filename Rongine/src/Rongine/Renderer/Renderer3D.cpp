#include "Rongpch.h"
#include "Renderer3D.h"
#include "RenderCommand.h" // 确保包含 RenderCommand
#include "Rongine/Scene/Entity.h" 
#include "Rongine/Scene/Components.h"
#include "Rongine/Renderer/BVH.h"

#include <glm/gtc/matrix_transform.hpp>
#include <array>
#include <glad/glad.h>

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
			{ ShaderDataType::Float,  "a_TilingFactor" },
			{ ShaderDataType::Int,    "a_FaceID" }
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

		s_Data.LineShader = Shader::create("assets/shaders/Line.glsl");

		s_Data.TextureSlots[0] = s_Data.WhiteTexture;

		// 初始化 Model 矩阵为单位矩阵，防止第一帧 Batch 渲染出错
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


		//线框
		s_Data.BatchLineVA = VertexArray::create();

		s_Data.BatchLineVB = VertexBuffer::create(s_Data.MaxLineVertices * sizeof(BatchLineVertex));
		s_Data.BatchLineVB->setLayout({
			{ ShaderDataType::Float3, "a_Position" },
			{ ShaderDataType::Float4, "a_Color" }
			});
		s_Data.BatchLineVA->addVertexBuffer(s_Data.BatchLineVB);

		s_Data.BatchLineVertexBufferBase = new BatchLineVertex[s_Data.MaxLineVertices];

		s_Data.BatchLineShader = Shader::create("assets/shaders/BatchLine.glsl");


		//  初始化光线追踪资源
		// 使用新的 Spec 创建高精度纹理
		TextureSpecification spec;
		spec.Width = 1280;
		spec.Height = 720;
		spec.Format = ImageFormat::RGBA32F; 
		s_Data.ComputeOutputTexture = Texture2D::create(spec);

		s_Data.RaytracingShader = ComputeShader::create("assets/shaders/Raytrace.glsl");
		s_Data.SpectralShader = ComputeShader::create("assets/shaders/SpectralRaytrace.glsl");
	}

	void Renderer3D::shutdown()
	{
		delete[] s_Data.CubeVertexBufferBase;
	}

	void Renderer3D::setSelection(int entityID, int faceID)
	{
		s_Data.SelectedEntityID = entityID;
		s_Data.SelectedFaceID = faceID;
	}

	void Renderer3D::setHover(int entityID, int faceID, int edgeID)
	{
		s_Data.HoveredEntityID = entityID;
		s_Data.HoveredFaceID = faceID;
		s_Data.HoveredEdgeID = edgeID;
	}

	int Renderer3D::getHoveredEntityID()
	{
		return s_Data.HoveredEntityID;
	}

	void Renderer3D::setSpectralRendering(bool enable) { s_Data.UseSpectralRendering = enable; }

	bool Renderer3D::isSpectralRendering() { return s_Data.UseSpectralRendering; }

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

		// Batch 渲染时，顶点已经在 CPU 变换过了，所以 GPU 的 u_Model 必须是 Identity
		s_Data.TextureShader->setMat4("u_Model", glm::mat4(1.0f));
		s_Data.TextureShader->setInt("u_EntityID", -1);
		s_Data.TextureShader->setInt("u_SelectedEntityID", -1);
		s_Data.TextureShader->setInt("u_HoveredEntityID", -1);
		s_Data.TextureShader->setFloat3("u_Albedo", glm::vec3(1.0f));
		s_Data.TextureShader->setFloat("u_Roughness", 0.5f);
		s_Data.TextureShader->setFloat("u_Metallic", 0.0f);

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

	void Renderer3D::beginLines(const PerspectiveCamera& camera)
	{
		s_Data.BatchLineShader->bind();
		s_Data.BatchLineShader->setMat4("u_ViewProjection", camera.getViewProjectionMatrix());

		glDisable(GL_DEPTH_TEST);

		s_Data.BatchLineVertexCount = 0;
		s_Data.BatchLineVertexBufferPtr = s_Data.BatchLineVertexBufferBase;
	}

	void Renderer3D::endLines()
	{
		if (s_Data.BatchLineVertexCount > 0)
		{
			uint32_t dataSize = (uint32_t)((uint8_t*)s_Data.BatchLineVertexBufferPtr - (uint8_t*)s_Data.BatchLineVertexBufferBase);
			s_Data.BatchLineVB->setData(s_Data.BatchLineVertexBufferBase, dataSize);

			s_Data.BatchLineVA->bind();
			RenderCommand::drawLines(s_Data.BatchLineVA, s_Data.BatchLineVertexCount);

			s_Data.Stats.DrawCalls++;
		}

		glEnable(GL_DEPTH_TEST);
	}

	void Renderer3D::drawLine(const glm::vec3& p0, const glm::vec3& p1, const glm::vec4& color)
	{
		if (s_Data.BatchLineVertexCount >= Renderer3DData::MaxLineVertices)
			return;

		s_Data.BatchLineVertexBufferPtr->Position = p0;
		s_Data.BatchLineVertexBufferPtr->Color = color;
		s_Data.BatchLineVertexBufferPtr++;

		s_Data.BatchLineVertexBufferPtr->Position = p1;
		s_Data.BatchLineVertexBufferPtr->Color = color;
		s_Data.BatchLineVertexBufferPtr++;

		s_Data.BatchLineVertexCount += 2;
	}

	void Renderer3D::drawGrid(const glm::mat4& transform, float size, int steps)
	{
		float stepSize = size / steps;

		glm::vec4 greyColor = { 0.6f, 0.6f, 0.6f, 0.5f };
		glm::vec4 redColor = { 0.8f, 0.2f, 0.2f, 1.0f };
		glm::vec4 greenColor = { 0.2f, 0.8f, 0.2f, 1.0f };

		// 1. 批量计算并写入灰色网格线
		for (int i = -steps; i <= steps; i++)
		{
			if (i == 0) continue;

			float pos = i * stepSize;

			// 本地坐标点 (延伸到负方向)
			glm::vec4 p1_local = { pos, -size, 0.0f, 1.0f }; // 竖线
			glm::vec4 p2_local = { pos,  size, 0.0f, 1.0f };
			glm::vec4 p3_local = { -size, pos, 0.0f, 1.0f }; // 横线
			glm::vec4 p4_local = { size, pos, 0.0f, 1.0f };

			drawLine(glm::vec3(transform * p1_local), glm::vec3(transform * p2_local), greyColor);
			drawLine(glm::vec3(transform * p3_local), glm::vec3(transform * p4_local), greyColor);
		}

		// 2. 写入十字坐标轴 (贯穿全屏)
		// X轴 (红): 从左到右 (-size 到 size)
		glm::vec3 xStart = glm::vec3(transform * glm::vec4(-size, 0.0f, 0.0f, 1.0f));
		glm::vec3 xEnd = glm::vec3(transform * glm::vec4(size, 0.0f, 0.0f, 1.0f));

		// Y轴 (绿): 从下到上 (-size 到 size)
		glm::vec3 yStart = glm::vec3(transform * glm::vec4(0.0f, -size, 0.0f, 1.0f));
		glm::vec3 yEnd = glm::vec3(transform * glm::vec4(0.0f, size, 0.0f, 1.0f));

		drawLine(xStart, xEnd, redColor);
		drawLine(yStart, yEnd, greenColor);
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
			s_Data.CubeVertexBufferPtr->FaceID = -1;

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
	//  Mesh Rendering
	// ===========================================
	void Renderer3D::drawModel(const Ref<VertexArray>& va, const glm::mat4& transform,int entityID, const MaterialComponent* material)
	{
		// 1. 如果批处理里有方块没画，先画掉 (flush 会使用 Identity Model 矩阵)
		//flush();

		s_Data.TextureShader->bind();

		// 2. 分别设置矩阵
		// u_Model: 物体的变换 (Shader 用它算 v_Position 和 v_Normal)
		s_Data.TextureShader->setMat4("u_Model", transform);

		// u_ViewProjection: 相机的 VP (保持 beginScene 设的值，不需要乘 transform)
		s_Data.TextureShader->setMat4("u_ViewProjection", s_Data.ViewProjection);

		s_Data.WhiteTexture->bind(0);

		s_Data.TextureShader->setInt("u_EntityID", entityID);

		s_Data.TextureShader->setInt("u_SelectedEntityID", s_Data.SelectedEntityID);
		s_Data.TextureShader->setInt("u_SelectedFaceID", s_Data.SelectedFaceID);

		s_Data.TextureShader->setInt("u_HoveredEntityID", s_Data.HoveredEntityID);
		s_Data.TextureShader->setInt("u_HoveredFaceID", s_Data.HoveredFaceID);

		if (material)
		{
			s_Data.TextureShader->setFloat3("u_Albedo", material->Albedo);
			s_Data.TextureShader->setFloat("u_Roughness", material->Roughness);
			s_Data.TextureShader->setFloat("u_Metallic", material->Metallic);
		}
		else
		{
			s_Data.TextureShader->setFloat3("u_Albedo", glm::vec3(1.0f));
			s_Data.TextureShader->setFloat("u_Roughness", 0.5f);
			s_Data.TextureShader->setFloat("u_Metallic", 0.0f);
		}

		va->bind();

		// 3. 显式传递 Index Count！
		uint32_t count = va->getIndexBuffer()->getCount();
		RenderCommand::drawIndexed(va, count);

		s_Data.Stats.DrawCalls++;

		// 4. 画完后，恢复 u_Model 为单位矩阵
		// 否则之后如果再调用 drawCube，方块会飞到错误的地方
		s_Data.TextureShader->setMat4("u_Model", glm::mat4(1.0f));
	}

	void Renderer3D::drawModel(Entity& en, const glm::mat4& transform,int entityID)
	{
		s_Data.TextureShader->bind();

		s_Data.TextureShader->setMat4("u_Model", transform);
		s_Data.TextureShader->setMat4("u_ViewProjection", s_Data.ViewProjection);

		s_Data.WhiteTexture->bind(0);

		if (en.HasComponent<MaterialComponent>())
		{
			const auto& mat = en.GetComponent<MaterialComponent>();

			s_Data.TextureShader->setFloat3("u_Albedo", mat.Albedo);
			s_Data.TextureShader->setFloat("u_Roughness", mat.Roughness);
			s_Data.TextureShader->setFloat("u_Metallic", mat.Metallic);
		}

		s_Data.TextureShader->setInt("u_EntityID", entityID);

		s_Data.TextureShader->setInt("u_SelectedEntityID", s_Data.SelectedEntityID);
		s_Data.TextureShader->setInt("u_SelectedFaceID", s_Data.SelectedFaceID);

		s_Data.TextureShader->setInt("u_HoveredEntityID", s_Data.HoveredEntityID);
		s_Data.TextureShader->setInt("u_HoveredFaceID", s_Data.HoveredFaceID);


		Ref<VertexArray> va = en.GetComponent<MeshComponent>().VA;
		va->bind();

		// 3. 显式传递 Index Count！
		uint32_t count = va->getIndexBuffer()->getCount();
		RenderCommand::drawIndexed(va, count);

		s_Data.Stats.DrawCalls++;

		//清理显存
		s_Data.TextureShader->setMat4("u_Model", glm::mat4(1.0f));

		s_Data.TextureShader->setFloat3("u_Albedo", glm::vec3(1.0f));
		s_Data.TextureShader->setFloat("u_Roughness", 0.5f);
		s_Data.TextureShader->setFloat("u_Metallic", 0.0f);
	}

	void Renderer3D::drawEdges(const Ref<VertexArray>& va, const glm::mat4& transform, const glm::vec4& color, int entityID, int selectedEdgeID)
	{
		if (!va) return;

		// 绑定线框 Shader
		s_Data.LineShader->bind();

		s_Data.LineShader->setMat4("u_ViewProjection", s_Data.ViewProjection);
		s_Data.LineShader->setMat4("u_Transform", transform);
		s_Data.LineShader->setFloat4("u_Color", color);

		s_Data.LineShader->setInt("u_EntityID", entityID);
		s_Data.LineShader->setInt("u_SelectedEdgeID", selectedEdgeID);

		s_Data.LineShader->setInt("u_HoveredEntityID", s_Data.HoveredEntityID);
		s_Data.LineShader->setInt("u_HoveredEdgeID", s_Data.HoveredEdgeID);

		va->bind();

		// 绘制线条
		// 获取第一个 VertexBuffer
		auto& vb = va->getVertexBuffers()[0];

		// 通过 Size / Stride 计算顶点数量
		// count = 总字节数 / 单个顶点的步长
		uint32_t vertexCount = vb->getSize() / vb->getLayout().getStride();
		RenderCommand::drawLines(va, vertexCount);
	}


	void Renderer3D::SetSpectralRange(float start, float end)
	{
		if (s_Data.SpectralStart != start || s_Data.SpectralEnd != end)
		{
			s_Data.SpectralStart = start;
			s_Data.SpectralEnd = end;

			s_Data.FrameIndex = 1;
		}
	}

	void Renderer3D::UploadSceneDataToGPU(Scene* scene)
	{
		// 1. 清空所有 Host 缓存
		s_Data.HostVertices.clear();
		s_Data.HostTriangles.clear();
		s_Data.HostMaterials.clear();
		s_Data.HostSpectralCurves.clear(); // [新增] 清空光谱数据

		auto view = scene->getRegistry().view<TransformComponent, MeshComponent>();

		for (auto entityHandle : view)
		{
			auto [tc, mesh] = view.get<TransformComponent, MeshComponent>(entityHandle);

			if (mesh.LocalVertices.empty() || mesh.LocalIndices.empty())
				continue;

			// ==================== 材质处理逻辑 ====================
			// 默认值初始化
			GPUMaterial gpuMat;
			// 初始化默认值
			gpuMat.AlbedoRoughness = { 0.8f, 0.8f, 0.8f, 0.5f }; // RGB, Roughness
			gpuMat.Metallic = 0.0f;
			gpuMat.Emission = 0.0f;
			gpuMat.SpectralIndex0 = -1; // -1 表示无效
			gpuMat.SpectralIndex1 = -1;
			gpuMat.Type = 0; // 默认 Diffuse
			gpuMat._pad1 = 0; gpuMat._pad2 = 0; gpuMat._pad3 = 0;

			Entity entity = { entityHandle, scene };

			// --- 1. 读取基础物理属性 (作为 Fallback 或混合参数) ---
			if (entity.HasComponent<MaterialComponent>())
			{
				auto& mat = entity.GetComponent<MaterialComponent>();
				gpuMat.AlbedoRoughness = { mat.Albedo.r, mat.Albedo.g, mat.Albedo.b, mat.Roughness };
				gpuMat.Metallic = mat.Metallic;
			}

			// --- 2. 读取光谱数据 ---
			if (entity.HasComponent<SpectralMaterialComponent>())
			{
				auto& specComp = entity.GetComponent<SpectralMaterialComponent>();

				// 设置材质类型 (0=Diffuse, 1=Conductor, 2=Dielectric)
				gpuMat.Type = (int)specComp.Type;

				// 辅助 Lambda: 上传一条曲线并返回索引
				auto UploadCurve = [&](const std::vector<float>& curveData) -> int {
					if (curveData.empty()) return -1;

					// 32点对齐
					int startIndex = (int)(s_Data.HostSpectralCurves.size() / 32);
					std::vector<float> aligned = curveData;
					if (aligned.size() < 32) aligned.resize(32, 0.0f);
					if (aligned.size() > 32) aligned.resize(32);

					s_Data.HostSpectralCurves.insert(s_Data.HostSpectralCurves.end(), aligned.begin(), aligned.end());
					return startIndex;
					};

				// 根据类型上传 Slot0 和 Slot1
				// Slot0: Reflectance / n / Transmission
				gpuMat.SpectralIndex0 = UploadCurve(specComp.SpectrumSlot0);

				// Slot1: k / IOR (只有金属和玻璃需要)
				if (specComp.Type != SpectralMaterialComponent::MaterialType::Diffuse) {
					gpuMat.SpectralIndex1 = UploadCurve(specComp.SpectrumSlot1);
				}
			}

			// 存入 HostMaterials
			uint32_t currentMatIndex = (uint32_t)s_Data.HostMaterials.size();
			s_Data.HostMaterials.push_back(gpuMat);			
			// ============================================================

			glm::mat4 transform = tc.GetTransform();
			glm::mat3 normalMatrix = glm::mat3(glm::transpose(glm::inverse(transform)));

			uint32_t vertexOffset = (uint32_t)s_Data.HostVertices.size();

			// --- 转换并合并顶点 ---
			for (const auto& v : mesh.LocalVertices)
			{
				GPUVertex gpuV;
				gpuV.Position = glm::vec3(transform * glm::vec4(v.Position, 1.0f));
				gpuV.Normal = glm::normalize(normalMatrix * v.Normal);
				gpuV.TexCoord = v.TexCoord;
				gpuV._pad1 = 0.0f; gpuV._pad2 = 0.0f; gpuV._pad3 = { 0.0f, 0.0f };

				s_Data.HostVertices.push_back(gpuV);
			}

			// --- 合并索引 ---
			for (size_t i = 0; i < mesh.LocalIndices.size(); i += 3)
			{
				if (i + 2 >= mesh.LocalIndices.size()) break;

				TriangleData tri;
				tri.v0 = mesh.LocalIndices[i + 0] + vertexOffset;
				tri.v1 = mesh.LocalIndices[i + 1] + vertexOffset;
				tri.v2 = mesh.LocalIndices[i + 2] + vertexOffset;
				tri.MaterialID = currentMatIndex;

				s_Data.HostTriangles.push_back(tri);
			}
		}

		// ==================== 上传 SSBO 数据 ====================

		if (s_Data.HostVertices.empty()) return;

		size_t vertSize = s_Data.HostVertices.size() * sizeof(GPUVertex);
		size_t triSize = s_Data.HostTriangles.size() * sizeof(TriangleData);
		size_t matSize = s_Data.HostMaterials.size() * sizeof(GPUMaterial);

		// 1. Vertices SSBO
		if (!s_Data.VerticesSSBO) s_Data.VerticesSSBO = ShaderStorageBuffer::create((uint32_t)vertSize, ShaderStorageBufferUsage::DynamicDraw);
		else s_Data.VerticesSSBO->resize((uint32_t)vertSize);
		s_Data.VerticesSSBO->bind(1);
		s_Data.VerticesSSBO->setData(s_Data.HostVertices.data(), (uint32_t)vertSize);

		// 2. Triangles SSBO
		if (!s_Data.TrianglesSSBO) s_Data.TrianglesSSBO = ShaderStorageBuffer::create((uint32_t)triSize, ShaderStorageBufferUsage::DynamicDraw);
		else s_Data.TrianglesSSBO->resize((uint32_t)triSize);
		s_Data.TrianglesSSBO->bind(2);
		s_Data.TrianglesSSBO->setData(s_Data.HostTriangles.data(), (uint32_t)triSize);

		// 3. Materials SSBO
		if (!s_Data.MaterialsSSBO) s_Data.MaterialsSSBO = ShaderStorageBuffer::create((uint32_t)matSize, ShaderStorageBufferUsage::DynamicDraw);
		else s_Data.MaterialsSSBO->resize((uint32_t)matSize);
		s_Data.MaterialsSSBO->bind(3);
		s_Data.MaterialsSSBO->setData(s_Data.HostMaterials.data(), (uint32_t)matSize);

		// 4.  Spectral Curves SSBO (Binding = 5)
		if (!s_Data.HostSpectralCurves.empty())
		{
			size_t curveSize = s_Data.HostSpectralCurves.size() * sizeof(float);

			if (!s_Data.SpectralCurvesSSBO)
				s_Data.SpectralCurvesSSBO = ShaderStorageBuffer::create((uint32_t)curveSize, ShaderStorageBufferUsage::DynamicDraw);
			else
				s_Data.SpectralCurvesSSBO->resize((uint32_t)curveSize);

			s_Data.SpectralCurvesSSBO->bind(5); //避免冲突
			s_Data.SpectralCurvesSSBO->setData(s_Data.HostSpectralCurves.data(), (uint32_t)curveSize);
		}
	}

	void Renderer3D::RenderComputeFrame(const PerspectiveCamera& camera, float time, bool resetAccumulation)
	{
		// 1. 帧数管理
		if (resetAccumulation)
			s_Data.FrameIndex = 1;
		else
			s_Data.FrameIndex++;

		Ref<ComputeShader> shader;
		if (s_Data.UseSpectralRendering)
			shader = s_Data.SpectralShader;
		else
			shader = s_Data.RaytracingShader;

		auto& outputTexture = s_Data.ComputeOutputTexture;
		auto& accumulationTexture = s_Data.AccumulationTexture;

		if (!shader || !outputTexture || !accumulationTexture) return;

		shader->bind();

		// 2. 绑定图像单元
		// Binding 0: 输出显示
		glBindImageTexture(0, outputTexture->getRendererID(), 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
		// Binding 4: 累积缓冲区
		glBindImageTexture(4, accumulationTexture->getRendererID(), 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);

		glm::mat4 invProj = glm::inverse(camera.getProjectionMatrix());
		glm::mat4 invView = glm::inverse(camera.getViewMatrix());

		// 3. 设置 Uniforms
		shader->setFloat("u_Time", time);
		shader->setMat4("u_InverseProjection", invProj);
		shader->setMat4("u_InverseView", invView);
		shader->setFloat3("u_CameraPos", camera.getPosition());
		shader->setInt("u_FrameIndex", s_Data.FrameIndex);

		shader->setFloat("u_LambdaMin", s_Data.SpectralStart);
		shader->setFloat("u_LambdaMax", s_Data.SpectralEnd);

		// 4. 绑定 SSBO
		if (s_Data.VerticesSSBO) s_Data.VerticesSSBO->bind(1);
		if (s_Data.TrianglesSSBO) s_Data.TrianglesSSBO->bind(2);
		if (s_Data.MaterialsSSBO) s_Data.MaterialsSSBO->bind(3);
		if (s_Data.SpectralCurvesSSBO) s_Data.SpectralCurvesSSBO->bind(5);

		// 5. 发射计算
		uint32_t width = outputTexture->getWidth();
		uint32_t height = outputTexture->getHeight();
		uint32_t groupX = (width + 8 - 1) / 8;
		uint32_t groupY = (height + 8 - 1) / 8;

		shader->dispatch(groupX, groupY, 1);
		shader->unbind();
	}

	Ref<Texture2D> Renderer3D::GetComputeOutputTexture()
	{
		return s_Data.ComputeOutputTexture;
	}

	void Renderer3D::ResizeComputeOutput(uint32_t width, uint32_t height)
	{
		// 1. 安全检查：如果尺寸没变，或者尺寸无效，直接返回
		if (s_Data.ComputeOutputTexture &&
			s_Data.ComputeOutputTexture->getWidth() == width &&
			s_Data.ComputeOutputTexture->getHeight() == height)
		{
			return;
		}

		if (width == 0 || height == 0) return;

		// 2. 重新创建高精度纹理
		TextureSpecification spec;
		spec.Width = width;
		spec.Height = height;
		spec.Format = ImageFormat::RGBA32F; // 必须保持 RGBA32F
		s_Data.ComputeOutputTexture = Texture2D::create(spec);

		// 创建累积纹理
		spec.Width = width;
		spec.Height = height;
		spec.Format = ImageFormat::RGBA32F;
		s_Data.AccumulationTexture = Texture2D::create(spec);

		s_Data.FrameIndex = 1;
	}

	void Renderer3D::BuildAccelerationStructures(Scene* scene)
	{
		// 1. 如果当前没有启用 BVH，直接返回，节省性能
		if (s_Data.CurrentAccelType != AccelType::BVH)
			return;

		// 计时开始 (用于性能分析)
		auto start = std::chrono::high_resolution_clock::now();

		// 2. 收集场景中所有的三角形
		// 注意：这里的遍历顺序必须与 UploadSceneDataToGPU 中上传 Triangles 的顺序严格一致！
		// 否则索引就会错乱，BVH 会指向错误的三角形。

		std::vector<BVHTriangle> worldTriangles;
		uint32_t globalTriIndex = 0; // 这是三角形在 GPU Triangles Buffer (Binding 2) 中的原始索引

		// 获取所有带 Mesh 和 Transform 的实体
		auto view = scene->getAllEntitiesWith<TransformComponent, MeshComponent>();

		for (auto entity : view)
		{
			auto [tc, mesh] = view.get<TransformComponent, MeshComponent>(entity);

			// 跳过空 Mesh
			if (mesh.LocalVertices.empty() || mesh.LocalIndices.empty())
				continue;

			glm::mat4 transform = tc.GetTransform();

			// 遍历当前 Mesh 的所有三角形
			// 假设是 Indexed Draw，步长为 3
			for (size_t i = 0; i < mesh.LocalIndices.size(); i += 3)
			{
				// 获取顶点索引
				uint32_t idx0 = mesh.LocalIndices[i];
				uint32_t idx1 = mesh.LocalIndices[i + 1];
				uint32_t idx2 = mesh.LocalIndices[i + 2];

				// 变换到世界坐标 (World Space)
				// BVH 必须基于世界坐标构建，因为光线是在世界空间漫游的
				glm::vec4 v0 = transform * glm::vec4(mesh.LocalVertices[idx0].Position, 1.0f);
				glm::vec4 v1 = transform * glm::vec4(mesh.LocalVertices[idx1].Position, 1.0f);
				glm::vec4 v2 = transform * glm::vec4(mesh.LocalVertices[idx2].Position, 1.0f);

				BVHTriangle tri;
				tri.V0 = glm::vec3(v0);
				tri.V1 = glm::vec3(v1);
				tri.V2 = glm::vec3(v2);

				// 计算质心 (用于构建时的空间划分)
				tri.Centroid = (tri.V0 + tri.V1 + tri.V2) / 3.0f;

				// 记录它在 GPU 三角形大数组里的原始 ID
				tri.Index = globalTriIndex++;

				worldTriangles.push_back(tri);
			}
		}

		// 如果没有三角形，就不构建了
		if (worldTriangles.empty()) return;

		// 3. 执行 BVH 构建 (CPU高计算量操作)
		BVHBuilder builder(worldTriangles);

		// 4. 获取构建结果
		const auto& nodes = builder.GetNodes();
		const auto& sortedIndices = builder.GetSortedIndices();

		s_Data.BVHNodeCount = (uint32_t)nodes.size();

		// 5. 上传节点数据 (Binding 6)
		uint32_t nodeBufferSize = (uint32_t)nodes.size() * sizeof(GPUBVHNode);

		if (!s_Data.BVHStorageBuffer || s_Data.BVHStorageBuffer->getSize() < nodeBufferSize)
		{
			s_Data.BVHStorageBuffer = ShaderStorageBuffer::create(nodeBufferSize, ShaderStorageBufferUsage::StaticDraw);
			s_Data.BVHStorageBuffer->bind(6);
		}
		s_Data.BVHStorageBuffer->setData(nodes.data(), nodeBufferSize);

		// 6. 上传索引映射表 (Binding 8)
		// Shader 遍历到叶子节点时，拿到的是 sortedIndices 里的索引，
		// 需要通过 sortedIndices[i] 查找到原始的 TriangleID
		uint32_t indexBufferSize = (uint32_t)sortedIndices.size() * sizeof(uint32_t);

		if (!s_Data.IndexMapBuffer || s_Data.IndexMapBuffer->getSize() < indexBufferSize)
		{
			s_Data.IndexMapBuffer = ShaderStorageBuffer::create(indexBufferSize, ShaderStorageBufferUsage::StaticDraw);
			s_Data.IndexMapBuffer->bind(8);
		}
		s_Data.IndexMapBuffer->setData(sortedIndices.data(), indexBufferSize);

		// 性能统计日志
		auto end = std::chrono::high_resolution_clock::now();
		float duration = std::chrono::duration<float, std::milli>(end - start).count();
		RONG_CORE_INFO("BVH Rebuilt: {0} Triangles, {1} Nodes in {2}ms", worldTriangles.size(), nodes.size(), duration);
	}

	void Renderer3D::setAccelType(const AccelType& acceltype)
	{
		s_Data.CurrentAccelType = acceltype;
	}

	AccelType Renderer3D::getAccelType()
	{
		return s_Data.CurrentAccelType;
	}

	int Renderer3D::getBVHNodeCount()
	{
		return s_Data.BVHNodeCount;
	}

	int Renderer3D::getOctreeNodeCount()
	{
		return s_Data.OctreeNodes.size();
	}

	Renderer3D::Statistics Renderer3D::getStatistics() { return s_Data.Stats; }
	void Renderer3D::resetStatistics() { memset(&s_Data.Stats, 0, sizeof(Statistics)); }
}
