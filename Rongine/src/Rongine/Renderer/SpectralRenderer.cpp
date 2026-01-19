#include "Rongpch.h"
#include "SpectralRenderer.h"
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
		// 注意：这里假设你有一个类似于 Texture2D::create(width, height) 的空纹理创建接口
		// 如果没有，你需要去实现一个能接收 void* data 的 Texture 构造函数
		if (m_FinalTexture) m_FinalTexture.reset(); // 释放旧的
		m_FinalTexture = Texture2D::create(m_Width, m_Height);
	}

	void SpectralRenderer::Render(const Scene& scene, const PerspectiveCamera& camera)
	{
		if (m_Width == 0 || m_Height == 0) return;

		// 创建一个行索引的集合: 0, 1, 2, ..., height-1
		std::vector<uint32_t> verticalIter(m_Height);
		std::iota(verticalIter.begin(), verticalIter.end(), 0);

		// 使用 std::execution::par 让所有 CPU 核心一起跑
		std::for_each(std::execution::par, verticalIter.begin(), verticalIter.end(),
			[this, &camera](uint32_t y)
			{
				for (uint32_t x = 0; x < m_Width; x++)
				{
					glm::vec4 color = PerPixel(x, y, m_Width, m_Height, camera);

					// 写入 Buffer
					m_ImageData[x + y * m_Width] = ConvertToRGBA(color);
				}
			});

		// 把 CPU 数据上传到 GPU
		m_FinalTexture->setData(m_ImageData.data(), m_ImageData.size() * sizeof(uint32_t));
	}

	glm::vec4 SpectralRenderer::PerPixel(uint32_t x, uint32_t y, uint32_t width, uint32_t height, const PerspectiveCamera& camera)
	{
		Ray ray;
		ray.Origin = camera.getPosition();

		// 计算射线方向 (从相机穿过像素)
		// 1. 归一化设备坐标 (NDC): [-1, 1]
		// 注意：y 需要反转，因为屏幕坐标 y 向下，而 OpenGL y 向上
		glm::vec2 coord = { (float)x / (float)width, (float)y / (float)height };
		coord = coord * 2.0f - 1.0f; // 0..1 -> -1..1

		// 2. 利用逆矩阵计算方向 (这是最通用的方法)
		// 目标点在 View Space 的远平面
		glm::vec4 target = camera.getInverseProjectionMatrix() * glm::vec4(coord.x, coord.y, 1.0f, 1.0f);

		// 变换到 World Space (只旋转，不位移，所以 w=0)
		glm::vec3 rayDir = glm::vec3(camera.getInverseViewMatrix() * glm::vec4(glm::normalize(glm::vec3(target) / target.w), 0));

		ray.Direction = glm::normalize(rayDir);

		// 发射光线
		return glm::vec4(TraceRay(ray), 1.0f);
	}

	glm::vec3 SpectralRenderer::TraceRay(const Ray& ray)
	{
		// --- 简单的球体求交测试 (Hello World) ---

		// 定义一个球
		glm::vec3 spherePos = { 0.0f, 0.0f, 0.0f }; // 原点
		float radius = 0.5f;

		// 求解方程: (bx^2 + by^2)t^2 + (2(axbx + ayby))t + (ax^2 + ay^2 - r^2) = 0
		// 简化版：a = dot(rayDir, rayDir) = 1 (因为归一化了)
		// b = 2 * dot(oc, rayDir)
		// c = dot(oc, oc) - r^2

		glm::vec3 oc = ray.Origin - spherePos;
		float a = glm::dot(ray.Direction, ray.Direction);
		float b = 2.0f * glm::dot(oc, ray.Direction);
		float c = glm::dot(oc, oc) - radius * radius;

		float discriminant = b * b - 4 * a * c;

		if (discriminant > 0.0f)
		{
			// 击中了球

			// 计算最近的交点 t
			// t = (-b - sqrt(delta)) / 2a
			float t = (-b - std::sqrt(discriminant)) / (2.0f * a);

			if (t > 0.0f) // 确保在相机前方
			{
				glm::vec3 hitPoint = ray.Origin + ray.Direction * t;
				glm::vec3 normal = glm::normalize(hitPoint - spherePos);

				// 简单的光照可视化：把法线映射到颜色 (Normal * 0.5 + 0.5)
				return normal * 0.5f + 0.5f;
			}
		}

		// 没击中：返回背景色 (黑色或天空色)
		return glm::vec3(0.1f, 0.1f, 0.1f);
	}

}