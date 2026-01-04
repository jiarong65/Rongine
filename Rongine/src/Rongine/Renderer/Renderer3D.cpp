#include "Rongpch.h"
#include "Renderer3D.h"
#include "Rongine/Renderer/VertexArray.h"
#include "Rongine/Renderer/Shader.h"
#include "Rongine/Renderer/RenderCommand.h" // 确保这里面有 drawIndexed
#include <glm/gtc/matrix_transform.hpp>

namespace Rongine {

	struct Renderer3DData
	{
		Ref<VertexArray> CubeVA;
		Ref<Shader> FlatColorShader;
	};

	static Renderer3DData s_Data;

	void Renderer3D::init()
	{
		s_Data.CubeVA = VertexArray::create();

		// --- 1. 定义立方体的 8 个顶点 (x, y, z) ---
		float vertices[] = {
			-0.5f, -0.5f,  0.5f, // 0: 左下前
			 0.5f, -0.5f,  0.5f, // 1: 右下前
			 0.5f,  0.5f,  0.5f, // 2: 右上前
			-0.5f,  0.5f,  0.5f, // 3: 左上前
			-0.5f, -0.5f, -0.5f, // 4: 左下后
			 0.5f, -0.5f, -0.5f, // 5: 右下后
			 0.5f,  0.5f, -0.5f, // 6: 右上后
			-0.5f,  0.5f, -0.5f  // 7: 左上后
		};

		Ref<VertexBuffer> cubeVB = VertexBuffer::create(vertices, sizeof(vertices));
		// Layout 必须和 Shader 中的 layout(location=0) 对应
		cubeVB->setLayout({
			{ ShaderDataType::Float3, "a_Position" }
			});
		s_Data.CubeVA->addVertexBuffer(cubeVB);

		// --- 2. 定义索引 (12 个三角形) ---
		uint32_t indices[] = {
			0, 1, 2, 2, 3, 0, // 前面
			1, 5, 6, 6, 2, 1, // 右面
			7, 6, 5, 5, 4, 7, // 后面
			4, 0, 3, 3, 7, 4, // 左面
			4, 5, 1, 1, 0, 4, // 底面
			3, 2, 6, 6, 7, 3  // 顶面
		};

		Ref<IndexBuffer> cubeIB = IndexBuffer::create(indices, sizeof(indices) / sizeof(uint32_t));
		s_Data.CubeVA->setIndexBuffer(cubeIB);

		// --- 3. 加载 Shader ---
		// 确保你的 Shader 文件名正确，比如叫 "FlatColor.glsl"
		s_Data.FlatColorShader = Shader::create("assets/shaders/FlatColor.glsl");
	}

	void Renderer3D::shutdown()
	{
	}

	void Renderer3D::beginScene(const PerspectiveCamera& camera)
	{
		s_Data.FlatColorShader->bind();
		// 传入相机矩阵
		s_Data.FlatColorShader->setMat4("u_ViewProjection", camera.getViewProjectionMatrix());
	}

	void Renderer3D::endScene()
	{
	}

	void Renderer3D::drawCube(const glm::vec3& position, const glm::vec3& size, const glm::vec4& color)
	{
		s_Data.FlatColorShader->bind();

		// 设置颜色
		s_Data.FlatColorShader->setFloat4("u_Color", color);

		// 计算模型变换矩阵 (Model Matrix)
		glm::mat4 transform = glm::translate(glm::mat4(1.0f), position) * glm::scale(glm::mat4(1.0f), size);

		s_Data.FlatColorShader->setMat4("u_Transform", transform);

		// 绘制
		s_Data.CubeVA->bind();

		uint32_t count = s_Data.CubeVA->getIndexBuffer()->getCount();
		RenderCommand::drawIndexed(s_Data.CubeVA, count);
	}
}