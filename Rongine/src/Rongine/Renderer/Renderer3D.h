#pragma once

#include "Rongine/Renderer/PerspectiveCamera.h"
#include "Rongine/Renderer/Texture.h"
#include "Rongine/Renderer/Renderer2D.h" 
#include "Rongine/Renderer/VertexArray.h"
#include "Rongine/Renderer/Shader.h"
#include "Rongine/Renderer/RenderCommand.h"
#include "Rongine/Scene/Scene.h"
#include "RenderTypes.h"
#include "Rongine/Renderer/ShaderStorageBuffer.h"
#include "Rongine/Renderer/ComputeShader.h"
#include "Rongine/Renderer/Framebuffer.h"

#include <glm/glm.hpp>

namespace Rongine {

	class Entity;
	struct MaterialComponent;

	class Renderer3D
	{
	public:
		static void init();
		static void shutdown();

		static void setSelection(int entityID, int faceID);
		static void setHover(int entityID, int faceID, int edgeID);

		static void setSpectralRendering(bool enable);
		static bool isSpectralRendering();

		static void beginScene(const PerspectiveCamera& camera);
		static void endScene();
		static void flush();

		static void beginLines(const PerspectiveCamera& camera);
		static void endLines();

		// --- 基础绘制 ---
		static void drawLine(const glm::vec3& p0, const glm::vec3& p1, const glm::vec4& color);
		static void drawGrid(const glm::mat4& transform, float size, int steps);

		static void drawCube(const glm::vec3& position, const glm::vec3& size, const glm::vec4& color);
		static void drawCube(const glm::vec3& position, const glm::vec3& size, const Ref<Texture2D>& texture, const glm::vec4& tintColor = glm::vec4(1.0f));

		// --- 旋转绘制  ---
		static void drawRotatedCube(const glm::vec3& position, const glm::vec3& size, float rotation, const glm::vec3& axis, const glm::vec4& color);
		static void drawRotatedCube(const glm::vec3& position, const glm::vec3& size, float rotation, const glm::vec3& axis, const Ref<Texture2D>& texture, const glm::vec4& tintColor = glm::vec4(1.0f));

		static void drawModel(const Ref<VertexArray>& va, const glm::mat4& transform = glm::mat4(1.0f),int entityID=-1, const MaterialComponent* material=nullptr);
		static void drawModel(Entity& en, const glm::mat4& transform = glm::mat4(1.0f), int entityID = -1);
		static void drawEdges(const Ref<VertexArray>& va, const glm::mat4& transform, const glm::vec4& color, int entityID, int selectedEdgeID = -1);




		// --- 统计信息 ---
		struct Statistics
		{
			uint32_t DrawCalls = 0;
			uint32_t CubeCount = 0;
			uint32_t GetTotalVertexCount() { return CubeCount * 24; } // 24 vertices per cube
			uint32_t GetTotalIndexCount() { return CubeCount * 36; }  // 36 indices per cube
		};

		// cs光追
		static void UploadSceneDataToGPU(Scene* scene);
		static void RenderComputeFrame(const PerspectiveCamera& camera,float time,bool resetAccumulation=false);
		static Ref<Texture2D> GetComputeOutputTexture();
		static void ResizeComputeOutput(uint32_t width, uint32_t height);

		static Statistics getStatistics();
		static void resetStatistics();

	private:
		static void flushAndReset();
	};



	struct Renderer3DData
	{
		static const uint32_t MaxCubes = 10000;
		static const uint32_t MaxVertices = MaxCubes * 24;
		static const uint32_t MaxIndices = MaxCubes * 36;
		static const uint32_t MaxTextureSlots = 32;
		static const uint32_t MaxLines = 10000;
		static const uint32_t MaxLineVertices = MaxLines * 2;

		int SelectedEntityID;
		int SelectedFaceID;

		int HoveredEntityID = -1;
		int HoveredFaceID = -1;
		int HoveredEdgeID = -1;

		Ref<VertexArray> CubeVA;
		Ref<VertexBuffer> CubeVB;
		Ref<Shader> TextureShader;
		Ref<Shader> LineShader;
		Ref<Texture2D> WhiteTexture;

		Ref<VertexArray> BatchLineVA;
		Ref<VertexBuffer> BatchLineVB;
		Ref<Shader> BatchLineShader;

		uint32_t CubeIndexCount = 0;
		CubeVertex* CubeVertexBufferBase = nullptr;
		CubeVertex* CubeVertexBufferPtr = nullptr;

		uint32_t BatchLineVertexCount = 0;
		BatchLineVertex* BatchLineVertexBufferBase = nullptr;
		BatchLineVertex* BatchLineVertexBufferPtr = nullptr;

		std::array<Ref<Texture2D>, MaxTextureSlots> TextureSlots;
		uint32_t TextureSlotIndex = 1;

		glm::vec4 CubeVertexPositions[24];
		glm::vec3 CubeVertexNormals[24];

		Renderer3D::Statistics Stats;

		glm::mat4 ViewProjection;

		// SSBO
		Ref<ShaderStorageBuffer> VerticesSSBO;
		Ref<ShaderStorageBuffer> TrianglesSSBO;
		Ref<ShaderStorageBuffer> MaterialsSSBO;

		// 临时缓存，避免每帧都重新分配 vector 内存
		std::vector<GPUVertex> HostVertices;
		std::vector<TriangleData> HostTriangles;
		std::vector<GPUMaterial> HostMaterials;

		Ref<Texture2D> ComputeOutputTexture; // 画布
		Ref<Texture2D> AccumulationTexture;  // 累加 
		Ref<ComputeShader> RaytracingShader; // 画笔
		Ref<ComputeShader> SpectralShader;   //光谱画笔

		uint32_t FrameIndex = 1;             // 帧数
		bool UseSpectralRendering = false;   // 光谱光追开关
	};
}