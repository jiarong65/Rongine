#include "Rongpch.h"
#include "Renderer2D.h"
#include "Rongine/Renderer/VertexArray.h"
#include "Rongine/Renderer/Shader.h"
#include "Platform/OpenGL/OpenGLShader.h"
#include "Rongine/Renderer/RenderCommand.h"
#include <glm/gtc/matrix_transform.hpp>


namespace Rongine {
	
	struct Renderer2DStorage {
		Ref<VertexArray> QuadVertexArray;
		Ref<Shader> FlatColorShader;
	};

	Renderer2DStorage* s_data =nullptr;

	void Renderer2D::init()
	{
		s_data = new Renderer2DStorage();

		s_data->QuadVertexArray = VertexArray::create();

		float squareVertices[3 * 4] = {
			-0.5f, -0.5f, 0.0f,
			 0.5f, -0.5f, 0.0f,
			 0.5f,  0.5f, 0.0f,
			-0.5f,  0.5f, 0.0f
		};

		Ref<VertexBuffer> squareVB;
		squareVB = VertexBuffer::create(squareVertices, sizeof(squareVertices));

		BufferLayout layout = {
			{ShaderDataType::Float3,"a_Positon"}
		};
		squareVB->setLayout(layout);
		s_data->QuadVertexArray->addVertexBuffer(squareVB);


		uint32_t squareIndices[6] = { 0,1,2,2,3,0 };

		Ref<IndexBuffer> squareIB;
		squareIB = IndexBuffer::create(squareIndices, sizeof(squareIndices) / sizeof(uint32_t));
		s_data->QuadVertexArray->setIndexBuffer(squareIB);

		s_data->FlatColorShader = Shader::create("assets/shaders/FlatColor.glsl");
	}

	void Renderer2D::shutdown()
	{
		delete s_data;
	}

	void Renderer2D::beginScene(const OrthographicCamera& camera)
	{
		std::dynamic_pointer_cast<OpenGLShader>(s_data->FlatColorShader)->bind();
		std::dynamic_pointer_cast<OpenGLShader>(s_data->FlatColorShader)->uploadUniformMat4("u_ViewProjection",camera.getViewProjectionMatrix());
	}

	void Renderer2D::endScene()
	{
	}

	void Renderer2D::drawQuad(const glm::vec2& position, const glm::vec2& size, const glm::vec4& color)
	{
		drawQuad(glm::vec3(position, 0.0f), size, color);
	}

	void Renderer2D::drawQuad(const glm::vec3& position, const glm::vec2& size, const glm::vec4& color)
	{
		std::dynamic_pointer_cast<OpenGLShader>(s_data->FlatColorShader)->bind();
		std::dynamic_pointer_cast<OpenGLShader>(s_data->FlatColorShader)->uploadUniformFloat4("u_Color", color);
		std::dynamic_pointer_cast<OpenGLShader>(s_data->FlatColorShader)->uploadUniformFloat3("a_Position", position);

		glm::mat4 transform = glm::translate(glm::mat4(1.0f),position)* glm::scale(glm::mat4(1.0f), glm::vec3(1.0f));
		std::dynamic_pointer_cast<OpenGLShader>(s_data->FlatColorShader)->uploadUniformMat4("u_Transform", transform);

		s_data->QuadVertexArray->bind();
		RenderCommand::drawIndexed(s_data->QuadVertexArray);
	}


}

