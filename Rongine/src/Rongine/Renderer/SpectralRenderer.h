#pragma once

#include <glm/glm.hpp>
#include <memory>
#include <vector>
#include "Rongine/Scene/Scene.h"
#include "Rongine/Renderer/PerspectiveCamera.h"
#include "Rongine/Renderer/Texture.h"
#include "Rongine/Scene/Components.h"

namespace Rongine {

	class SpectralRenderer
	{
	public:
		SpectralRenderer();
		~SpectralRenderer() = default;

		void OnResize(uint32_t width, uint32_t height);

		void Render(Scene& scene, const PerspectiveCamera& camera);

		uint32_t GetFinalTextureID() const { return m_FinalTexture->getRendererID(); }

	private:
		// 一个简单的光线结构体
		struct Ray
		{
			glm::vec3 Origin;
			glm::vec3 Direction;
		};
		struct HitPayload
		{
			float HitDistance = -1.0f; // < 0 表示没打中
			glm::vec3 WorldPosition;
			glm::vec3 WorldNormal;
			int EntityID = -1;
		};

		//渲染每一个像素
		glm::vec4 PerPixel(uint32_t x, uint32_t y, uint32_t width, uint32_t height, const PerspectiveCamera& camera, Scene& scene);

		HitPayload TraceRay(const Ray& ray, Scene& scene);

		//光线-三角形求交辅助函数
		bool RayTriangleIntersect(const Ray& ray,
			const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2,
			float& t, glm::vec3& normal);

	private:
		Ref<Texture2D> m_FinalTexture;
		std::vector<uint32_t> m_ImageData; // CPU 端的像素 Buffer (RGBA8)

		uint32_t m_Width = 0, m_Height = 0;
	};

}