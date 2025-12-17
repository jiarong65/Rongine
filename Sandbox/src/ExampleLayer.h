#pragma once
#include <Rongine.h>

#include <glm/vec3.hpp> // glm::vec3
#include <glm/vec4.hpp> // glm::vec4
#include <glm/mat4x4.hpp> // glm::mat4
#include <glm/gtc/matrix_transform.hpp>

#include "Platform/OpenGL/OpenGLShader.h"
#include <glm/gtc/type_ptr.hpp>

#include "imgui/imgui.h"
#include "Sandbox2D.h"

class ExampleLayer :public Rongine::Layer
{
public:
	ExampleLayer()
		:Layer("example"), m_cameraContorller(1280.0f / 720.0f)
	{
		m_vertexArray = Rongine::VertexArray::create();

		float vertices[5 * 4] = {
			-0.5f, -0.5f, 0.0f, 0.0f, 0.0f,
			 0.5f, -0.5f, 0.0f, 1.0f, 0.0f,
			 0.5f,  0.5f, 0.0f, 1.0f, 1.0f,
			-0.5f,  0.5f, 0.0f, 0.0f, 1.0f
		};

		Rongine::Ref<Rongine::VertexBuffer> vertexBuffer;
		vertexBuffer = Rongine::VertexBuffer::create(vertices, sizeof(vertices));

		Rongine::BufferLayout layout = {
			{Rongine::ShaderDataType::Float3,"a_Positon"},
			{Rongine::ShaderDataType::Float2,"a_TexCoord"}
		};
		vertexBuffer->setLayout(layout);
		m_vertexArray->addVertexBuffer(vertexBuffer);


		uint32_t indices[6] = { 0,1,2,2,3,0 };

		Rongine::Ref<Rongine::IndexBuffer> indexBuffer;
		indexBuffer = Rongine::IndexBuffer::create(indices, sizeof(indices) / sizeof(uint32_t));
		m_vertexArray->setIndexBuffer(indexBuffer);

		//////////////////////////////start
		m_squareVA = Rongine::VertexArray::create();

		float squareVertices[3 * 4] = {
			-0.5f, -0.5f, 0.0f,
			 0.5f, -0.5f, 0.0f,
			 0.5f,  0.5f, 0.0f,
			-0.5f,  0.5f, 0.0f
		};

		Rongine::Ref<Rongine::VertexBuffer> squareVB;
		squareVB = Rongine::VertexBuffer::create(squareVertices, sizeof(squareVertices));

		squareVB->setLayout({
			{ Rongine::ShaderDataType::Float3, "a_Position" }
			});
		m_squareVA->addVertexBuffer(squareVB);


		uint32_t squareIndices[6] = { 0, 1, 2, 2, 3, 0 };

		Rongine::Ref<Rongine::IndexBuffer> squareIB;
		squareIB = Rongine::IndexBuffer::create(squareIndices, sizeof(squareIndices) / sizeof(uint32_t));
		m_squareVA->setIndexBuffer(squareIB);

		//////////////////////////////////end


		m_shaderLibrary.load("assets/shaders/Texture.glsl");

		m_texture = Rongine::Texture2D::create("assets/textures/Checkerboard.png");
		m_chernoLogoTexture = Rongine::Texture2D::create("assets/textures/ChernoLogo.png");

		auto& textureShader = m_shaderLibrary.get("Texture");

		std::dynamic_pointer_cast<Rongine::OpenGLShader>(textureShader)->bind();
		std::dynamic_pointer_cast<Rongine::OpenGLShader>(textureShader)->uploadUniformInt("u_Texture", 0);


		/////////////////////////////////start
		std::string flatColorShaderVertexSrc = R"(
			#version 330 core
			
			layout(location = 0) in vec3 a_Position;

			uniform mat4 u_ViewProjection;
			uniform mat4 u_Transform;

			out vec3 v_Position;

			void main()
			{
				v_Position = a_Position;
				gl_Position = u_ViewProjection*u_Transform*vec4(a_Position,1.0f);	
			}
		)";

		std::string flatColorShaderFragmentSrc = R"(
			#version 330 core
			
			layout(location = 0) out vec4 color;

			in vec3 v_Position;

			uniform vec3 u_Color;

			void main()
			{
				color = vec4(u_Color, 1.0);
			}
		)";

		m_flatColorShader = Rongine::Shader::create("FlatColor", flatColorShaderVertexSrc, flatColorShaderFragmentSrc);
		/////////////////////////////////end
	}

	void onUpdate(Rongine::Timestep ts) override
	{
		m_cameraContorller.onUpdate(ts);

		if (Rongine::Input::isKeyPressed(Rongine::Key::J))
			m_squarePosition.x -= m_squareMovedSpeed * ts;
		else if (Rongine::Input::isKeyPressed(Rongine::Key::L))
			m_squarePosition.x += m_squareMovedSpeed * ts;
		if (Rongine::Input::isKeyPressed(Rongine::Key::I))
			m_squarePosition.y += m_squareMovedSpeed * ts;
		else if (Rongine::Input::isKeyPressed(Rongine::Key::K))
			m_squarePosition.y -= m_squareMovedSpeed * ts;

		Rongine::RenderCommand::setColor({ 0.1f, 0.1f, 0.1f, 1 });
		Rongine::RenderCommand::clear();

		Rongine::Renderer::beginScene(m_cameraContorller.getCamera());

		glm::mat4 scale = glm::scale(glm::mat4(1.0f), glm::vec3(0.1f));

		//////////////////start

		Rongine::Ref<Rongine::OpenGLShader> shader_ptr = std::dynamic_pointer_cast<Rongine::OpenGLShader>(m_flatColorShader);
		shader_ptr->bind();
		shader_ptr->uploadUniformFloat3("u_Color", m_squareColor);

		for (int y = 0; y < 20; y++)
		{
			for (int x = 0; x < 20; x++)
			{
				glm::vec3 gridOffset(x * 0.11f, y * 0.11f, 0.0f);
				glm::vec3 pos = m_squarePosition + gridOffset;
				glm::mat4 transform = glm::translate(glm::mat4(1.0f), pos) * scale;
				Rongine::Renderer::submit(std::dynamic_pointer_cast<Rongine::OpenGLShader>(m_flatColorShader), m_squareVA, transform);
			}
		}
		Rongine::Renderer::submit(m_flatColorShader, m_squareVA, scale);
		//////////////////end

		auto& textureShader = m_shaderLibrary.get("Texture");

		m_texture->bind();
		Rongine::Renderer::submit(textureShader, m_vertexArray, glm::scale(glm::mat4(1.0f), glm::vec3(1.5f)));

		m_chernoLogoTexture->bind();
		Rongine::Renderer::submit(textureShader, m_vertexArray, glm::scale(glm::mat4(1.0f), glm::vec3(1.5f)));

		//Rongine::Renderer::submit(m_shader, m_vertexArray);

		Rongine::Renderer::endScene();
	}

	void onEvent(Rongine::Event& e) override
	{
		m_cameraContorller.onEvent(e);
	}

	void onImGuiRender()
	{
		ImGui::Begin("Setting");
		ImGui::ColorEdit3("Square Color", glm::value_ptr(m_squareColor));
		ImGui::End();
	}

private:
	Rongine::ShaderLibray m_shaderLibrary;
	Rongine::Ref<Rongine::VertexArray> m_vertexArray;

	Rongine::Ref<Rongine::Shader> m_flatColorShader;
	Rongine::Ref<Rongine::VertexArray> m_squareVA;

	Rongine::Ref<Rongine::Texture2D> m_texture, m_chernoLogoTexture;


	Rongine::OrthographicCameraController m_cameraContorller;

	glm::vec3 m_squareColor = { 0.2f, 0.3f, 0.8f };
	glm::vec3 m_squarePosition = { 0.0f,0.0f,0.0f };

	float m_squareMovedSpeed = 3.0f;
};

