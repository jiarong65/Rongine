#pragma once

#include "Rongine/Renderer/PerspectiveCamera.h"
#include "Rongine/Renderer/Texture.h"
#include "Rongine/Renderer/Renderer2D.h" 
#include "Rongine/Renderer/VertexArray.h"
#include "Rongine/Renderer/Shader.h"
#include "Rongine/Renderer/RenderCommand.h"

#include <glm/glm.hpp>

namespace Rongine {

	class Renderer3D
	{
	public:
		static void init();
		static void shutdown();

		static void setSelection(int entityID, int faceID);

		static void beginScene(const PerspectiveCamera& camera);
		static void endScene();
		static void flush();

		// --- 基础绘制 ---
		static void drawCube(const glm::vec3& position, const glm::vec3& size, const glm::vec4& color);
		static void drawCube(const glm::vec3& position, const glm::vec3& size, const Ref<Texture2D>& texture, const glm::vec4& tintColor = glm::vec4(1.0f));

		// --- 旋转绘制 (新增) ---
		static void drawRotatedCube(const glm::vec3& position, const glm::vec3& size, float rotation, const glm::vec3& axis, const glm::vec4& color);
		static void drawRotatedCube(const glm::vec3& position, const glm::vec3& size, float rotation, const glm::vec3& axis, const Ref<Texture2D>& texture, const glm::vec4& tintColor = glm::vec4(1.0f));

		static void drawModel(const Ref<VertexArray>& va, const glm::mat4& transform = glm::mat4(1.0f),int entityID=-1);
		static void drawEdges(const Ref<VertexArray>& va, const glm::mat4& transform, const glm::vec4& color = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));


		// --- 统计信息 ---
		struct Statistics
		{
			uint32_t DrawCalls = 0;
			uint32_t CubeCount = 0;
			uint32_t GetTotalVertexCount() { return CubeCount * 24; } // 24 vertices per cube
			uint32_t GetTotalIndexCount() { return CubeCount * 36; }  // 36 indices per cube
		};

		static Statistics getStatistics();
		static void resetStatistics();

	private:
		static void flushAndReset();
	};

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

	struct Renderer3DData
	{
		static const uint32_t MaxCubes = 10000;
		static const uint32_t MaxVertices = MaxCubes * 24;
		static const uint32_t MaxIndices = MaxCubes * 36;
		static const uint32_t MaxTextureSlots = 32;

		int SelectedEntityID;
		int SelectedFaceID;

		Ref<VertexArray> CubeVA;
		Ref<VertexBuffer> CubeVB;
		Ref<Shader> TextureShader;
		Ref<Shader> LineShader;
		Ref<Texture2D> WhiteTexture;

		uint32_t CubeIndexCount = 0;
		CubeVertex* CubeVertexBufferBase = nullptr;
		CubeVertex* CubeVertexBufferPtr = nullptr;

		std::array<Ref<Texture2D>, MaxTextureSlots> TextureSlots;
		uint32_t TextureSlotIndex = 1;

		glm::vec4 CubeVertexPositions[24];
		glm::vec3 CubeVertexNormals[24];

		Renderer3D::Statistics Stats;

		glm::mat4 ViewProjection;
	};


}