#include "Rongpch.h"
#include "SpectralRenderer.h"
#include "Rongine/Scene/Entity.h"
#include <random>
#include <execution>// C++17 并行算法
#include <numeric>// for std::iota


namespace Rongine {

	// 辅助工具：把 float (0.0-1.0) 转成 RGBA8 (0-255) 并打包成 uint32
	static uint32_t ConvertToRGBA(const glm::vec4& color)
	{
		uint8_t r = (uint8_t)(glm::clamp(color.r, 0.0f, 1.0f) * 255.0f);
		uint8_t g = (uint8_t)(glm::clamp(color.g, 0.0f, 1.0f) * 255.0f);
		uint8_t b = (uint8_t)(glm::clamp(color.b, 0.0f, 1.0f) * 255.0f);
		uint8_t a = (uint8_t)(glm::clamp(color.a, 0.0f, 1.0f) * 255.0f);
		return (a << 24) | (b << 16) | (g << 8) | r;
	}

	SpectralRenderer::SpectralRenderer()
	{
	}

	void SpectralRenderer::OnResize(uint32_t width, uint32_t height)
	{
		if (m_Width == width && m_Height == height) return;

		m_Width = width;
		m_Height = height;

		// 重新分配内存
		m_ImageData.resize(m_Width * m_Height);

		// 创建或重建 GPU 纹理
		if (m_FinalTexture) m_FinalTexture.reset(); // 释放旧的
		m_FinalTexture = Texture2D::create(m_Width, m_Height);
	}

	void SpectralRenderer::Render( Scene& scene, const PerspectiveCamera& camera)
	{
		if (m_Width == 0 || m_Height == 0) return;

		// 创建一个行索引的集合: 0, 1, 2, ..., height-1
		std::vector<uint32_t> verticalIter(m_Height);
		std::iota(verticalIter.begin(), verticalIter.end(), 0);

		// 使用 std::execution::par 让所有 CPU 核心一起跑
		std::for_each(std::execution::par, verticalIter.begin(), verticalIter.end(),
			[this, &camera,&scene](uint32_t y)
			{
				for (uint32_t x = 0; x < m_Width; x++)
				{
					glm::vec4 color = PerPixel(x, y, m_Width, m_Height, camera,scene);

					// 写入 Buffer
					m_ImageData[x + y * m_Width] = ConvertToRGBA(color);
				}
			});

		// 把 CPU 数据上传到 GPU
		m_FinalTexture->setData(m_ImageData.data(), m_ImageData.size() * sizeof(uint32_t));
	}

	// 计算像素
	glm::vec4 SpectralRenderer::PerPixel(uint32_t x, uint32_t y, uint32_t width, uint32_t height, const PerspectiveCamera& camera, Scene& scene)
	{
		Ray ray;
		ray.Origin = camera.getPosition();

		glm::vec2 coord = { (float)x / (float)width, (float)y / (float)height };
		coord = coord * 2.0f - 1.0f;

		glm::vec4 target = camera.getInverseProjectionMatrix() * glm::vec4(coord.x, coord.y, 1.0f, 1.0f);
		glm::vec3 rayDir = glm::vec3(camera.getInverseViewMatrix() * glm::vec4(glm::normalize(glm::vec3(target) / target.w), 0));

		ray.Direction = glm::normalize(rayDir);

		HitPayload payload = TraceRay(ray, scene);

		if (payload.HitDistance < 0.0f)
		{
			return glm::vec4(0.1f, 0.1f, 0.1f, 1.0f); // 背景色
		}

		// ===========================================================
		// 简单的光照计算 (Simple Lighting)
		// ===========================================================

		// 1. 定义光源 (比如从右上方射下来的白光)
		glm::vec3 lightDir = glm::normalize(glm::vec3(-1.0f, -1.0f, -1.0f)); // 光的方向 (指向光源的反方向)
		glm::vec3 lightColor = { 1.0f, 1.0f, 1.0f };

		// 2. 准备向量
		glm::vec3 normal = glm::normalize(payload.WorldNormal);
		glm::vec3 viewDir = glm::normalize(camera.getPosition() - payload.WorldPosition);

		// --- A. 漫反射 (Diffuse) ---
		// 塑料和金属都有漫反射，但金属的漫反射通常很弱（甚至为0，全黑）
		// N dot L
		float diff = glm::max(glm::dot(normal, -lightDir), 0.0f);
		glm::vec3 diffuse = diff * payload.Albedo;

		// --- B. 高光 (Specular) ---
		// 使用简单的反射向量算法
		glm::vec3 reflectDir = glm::reflect(lightDir, normal);

		// 粗糙度转光泽度 (Roughness -> Shininess)
		// 粗糙度 0 -> 非常亮 (指数大), 粗糙度 1 -> 非常散 (指数小)
		float shininess = (1.0f - payload.Roughness) * 256.0f;

		float spec = std::pow(glm::max(glm::dot(viewDir, reflectDir), 0.0f), shininess);
		glm::vec3 specular = spec * lightColor;

		// --- C. 材质混合 (Mix) ---
		glm::vec3 finalColor;

		if (payload.Metallic > 0.5f)
		{
			// 金属模式：
			// 漫反射几乎没有 (变黑)，原本的 Albedo 变成了高光的颜色 (有色高光)
			// 简单的金属近似：Albedo 作用于 Specular
			glm::vec3 kS = payload.Albedo;
			glm::vec3 kD = glm::vec3(0.0f); // 金属几乎没漫反射

			finalColor = kD * diff + specular * kS;
		}
		else
		{
			// 塑料/非金属模式：
			// 高光通常是白色的，Albedo 作用于漫反射
			float specularIntensity = 0.5f; // 塑料的高光强度通常固定
			finalColor = diffuse + specular * specularIntensity;
		}

		// --- D. 环境光 (Ambient) ---
		// 稍微加一点底色，防止背光面全黑
		glm::vec3 ambient = payload.Albedo * 0.1f;
		finalColor += ambient;

		// 简单的 Gamma 矫正 
		 finalColor = glm::pow(finalColor, glm::vec3(1.0f / 2.2f));

		return glm::vec4(finalColor, 1.0f);
	}

	// Möller–Trumbore ray-triangle intersection algorithm
	bool SpectralRenderer::RayTriangleIntersect(const Ray& ray,
		const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2,
		float& t, glm::vec3& normal)
	{
		const float EPSILON = 0.0000001f;
		glm::vec3 edge1 = v1 - v0;
		glm::vec3 edge2 = v2 - v0;
		glm::vec3 h = glm::cross(ray.Direction, edge2);
		float a = glm::dot(edge1, h);

		if (a > -EPSILON && a < EPSILON) return false; // 光线平行于三角形

		float f = 1.0f / a;
		glm::vec3 s = ray.Origin - v0;
		float u = f * glm::dot(s, h);
		if (u < 0.0f || u > 1.0f) return false;

		glm::vec3 q = glm::cross(s, edge1);
		float v = f * glm::dot(ray.Direction, q);
		if (v < 0.0f || u + v > 1.0f) return false;

		float currentT = f * glm::dot(edge2, q);
		if (currentT > EPSILON) // 只有 t > 0 才算击中（前方）
		{
			t = currentT;
			// 简单的面法线 (Flat Shading)
			// 如果要光滑着色，需要用 u,v 插值顶点法线
			normal = glm::normalize(glm::cross(edge1, edge2));
			return true;
		}
		return false;
	}

	SpectralRenderer::HitPayload SpectralRenderer::TraceRay(const Ray& ray, Scene& scene)
	{
		HitPayload payload;
		payload.HitDistance = std::numeric_limits<float>::max(); // 初始化为无穷远

		// 1. 获取场景中所有带 MeshComponent 的物体
		auto view = scene.getAllEntitiesWith<TransformComponent, MeshComponent>();

		// 2. 遍历所有物体 (Brute Force，以后用 BVH 加速)
		for (auto entityHandle : view)
		{
			auto [tc, mesh] = view.get<TransformComponent, MeshComponent>(entityHandle);

			// 跳过没有顶点数据的
			if (mesh.LocalVertices.empty()) continue;

			// 获取模型矩阵 (Model Matrix)
			glm::mat4 transform = tc.GetTransform();

			// 提取法线矩阵 (用于变换法线，防止非均匀缩放导致法线错误)
			glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(transform)));

			// 3. 遍历物体 meshes 里的所有三角形
			// LocalVertices 是 CubeVertex 结构体
			size_t numIndices = mesh.LocalIndices.size();
			for (size_t i = 0; i < numIndices; i += 3)
			{
				// 1. 从索引数组拿顶点下标
				uint32_t idx0 = mesh.LocalIndices[i];
				uint32_t idx1 = mesh.LocalIndices[i + 1];
				uint32_t idx2 = mesh.LocalIndices[i + 2];

				// 2. 用下标去顶点数组拿数据
				glm::vec3 localV0 = mesh.LocalVertices[idx0].Position;
				glm::vec3 localV1 = mesh.LocalVertices[idx1].Position;
				glm::vec3 localV2 = mesh.LocalVertices[idx2].Position;

				// 3. 变换到世界坐标
				glm::vec3 worldV0 = glm::vec3(transform * glm::vec4(localV0, 1.0f));
				glm::vec3 worldV1 = glm::vec3(transform * glm::vec4(localV1, 1.0f));
				glm::vec3 worldV2 = glm::vec3(transform * glm::vec4(localV2, 1.0f));

				float t = 0.0f;
				glm::vec3 n;

				if (RayTriangleIntersect(ray, worldV0, worldV1, worldV2, t, n))
				{
					if (t < payload.HitDistance)
					{
						payload.HitDistance = t;
						payload.EntityID = (int)(uint32_t)entityHandle;
						payload.WorldPosition = ray.Origin + ray.Direction * t;
						payload.WorldNormal = n;
					}

					Entity entity = { entityHandle, &scene };
					if (entity.HasComponent<MaterialComponent>())
					{
						const auto& material = entity.GetComponent<MaterialComponent>();
						payload.Albedo = material.Albedo;
						payload.Roughness = material.Roughness;
						payload.Metallic = material.Metallic;
					}
					else
					{
						// 默认材质 (灰色塑料)
						payload.Albedo = { 0.8f, 0.8f, 0.8f };
						payload.Roughness = 0.5f;
						payload.Metallic = 0.0f;
					}
				}
			}
		}

		// 如果没打中任何东西，距离重置为 -1
		if (payload.HitDistance == std::numeric_limits<float>::max())
			payload.HitDistance = -1.0f;

		return payload;
	}

}