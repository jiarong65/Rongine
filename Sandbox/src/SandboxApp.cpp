#include "Rongpch.h"
#include <Rongine.h>

#include <glm/vec3.hpp> // glm::vec3
#include <glm/vec4.hpp> // glm::vec4
#include <glm/mat4x4.hpp> // glm::mat4
#include <glm/gtc/matrix_transform.hpp> // glm::translate, glm::rotate, glm::scale, glm::perspective

#include "Platform/OpenGL/OpenGLShader.h"
#include <glm/gtc/type_ptr.hpp>

#include "imgui/imgui.h"

class ExampleLayer :public Rongine::Layer
{
public:
	ExampleLayer()
		:Layer("example"), m_camera(-1.6f, 1.6f, -0.9f, 0.9f)
	{
		m_vertexArray.reset(Rongine::VertexArray::create());

		float vertices[5 * 4] = {
			-0.5f, -0.5f, 0.0f, 0.0f, 0.0f,
			 0.5f, -0.5f, 0.0f, 1.0f, 0.0f,
			 0.5f,  0.5f, 0.0f, 1.0f, 1.0f,
			-0.5f,  0.5f, 0.0f, 0.0f, 1.0f
		};

		Rongine::Ref<Rongine::VertexBuffer> vertexBuffer;
		vertexBuffer.reset(Rongine::VertexBuffer::create(vertices, sizeof(vertices)));

		Rongine::BufferLayout layout = {
			{Rongine::ShaderDataType::Float3,"a_Positon"},
			{Rongine::ShaderDataType::Float2,"a_TexCoord"}
		};
		vertexBuffer->setLayout(layout);
		m_vertexArray->addVertexBuffer(vertexBuffer);


		uint32_t indices[6] = { 0,1,2,2,3,0 };

		Rongine::Ref<Rongine::IndexBuffer> indexBuffer;
		indexBuffer.reset(Rongine::IndexBuffer::create(indices, sizeof(indices) / sizeof(uint32_t)));
		m_vertexArray->setIndexBuffer(indexBuffer);

		//////////////////////////////start
		m_squareVA.reset(Rongine::VertexArray::create());

		float squareVertices[3 * 4] = {
			-0.5f, -0.5f, 0.0f,
			 0.5f, -0.5f, 0.0f,
			 0.5f,  0.5f, 0.0f,
			-0.5f,  0.5f, 0.0f
		};

		Rongine::Ref<Rongine::VertexBuffer> squareVB;
		squareVB.reset(Rongine::VertexBuffer::create(squareVertices, sizeof(squareVertices)));

		squareVB->setLayout({
			{ Rongine::ShaderDataType::Float3, "a_Position" }
			});
		m_squareVA->addVertexBuffer(squareVB);


		uint32_t squareIndices[6] = { 0, 1, 2, 2, 3, 0 };

		Rongine::Ref<Rongine::IndexBuffer> squareIB;
		squareIB.reset(Rongine::IndexBuffer::create(squareIndices, sizeof(squareIndices) / sizeof(uint32_t)));
		m_squareVA->setIndexBuffer(squareIB);

		//////////////////////////////////end

		std::string textureShaderVertexSrc = R"(
			#version 330 core
			
			layout(location = 0) in vec3 a_Position;
			layout(location = 1) in vec2 a_TexCoord;

			uniform mat4 u_ViewProjection;
			uniform mat4 u_Transform;

			out vec3 v_Position;
			out vec2 v_TexCoord;

			void main()
			{
				v_Position = a_Position;
				v_TexCoord=a_TexCoord;
				gl_Position = u_ViewProjection*u_Transform*vec4(a_Position,1.0f);	
			}
		)";

		std::string textureShaderfragmentSrc = R"(
			#version 330 core
			
			layout(location = 0) out vec4 color;

			in vec3 v_Position;
			in vec2 v_TexCoord;

			uniform sampler2D u_Texture;

			void main()
			{
				color = texture(u_Texture,v_TexCoord);
			}
		)";

		m_textureShader.reset(Rongine::Shader::create(textureShaderVertexSrc, textureShaderfragmentSrc));

		m_texture = Rongine::Texture2D::create("assets/textures/Checkerboard.png");

		std::dynamic_pointer_cast<Rongine::OpenGLShader>(m_textureShader)->bind();
		std::dynamic_pointer_cast<Rongine::OpenGLShader>(m_textureShader)->uploadUniformInt("u_Texture", 0);


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

		m_flatColorShader.reset(Rongine::Shader::create(flatColorShaderVertexSrc, flatColorShaderFragmentSrc));
		/////////////////////////////////end
	}

	void onUpdate(Rongine::Timestep ts) override
	{
		RONG_CLIENT_TRACE("Timestep is {0} ms", ts.getMilliseconds());

		if (Rongine::Input::isKeyPressed(Rongine::Key::Left))
			m_cameraPosition.x -= m_cameraMoveSpeed * ts;
		else if (Rongine::Input::isKeyPressed(Rongine::Key::Right))
			m_cameraPosition.x += m_cameraMoveSpeed * ts;
		if (Rongine::Input::isKeyPressed(Rongine::Key::Up))
			m_cameraPosition.y += m_cameraMoveSpeed * ts;
		else if(Rongine::Input::isKeyPressed(Rongine::Key::Down))
			m_cameraPosition.y -= m_cameraMoveSpeed * ts;

		if(Rongine::Input::isKeyPressed(Rongine::Key::A))
			m_cameraRotation -= m_cameraRotationSpeed * ts;
		else if(Rongine::Input::isKeyPressed(Rongine::Key::D))
			m_cameraRotation += m_cameraRotationSpeed * ts;

		if (Rongine::Input::isKeyPressed(Rongine::Key::J))
			m_squarePosition.x -= m_cameraMoveSpeed * ts;
		else if (Rongine::Input::isKeyPressed(Rongine::Key::L))
			m_squarePosition.x += m_cameraMoveSpeed * ts;
		if (Rongine::Input::isKeyPressed(Rongine::Key::I))
			m_squarePosition.y += m_cameraMoveSpeed * ts;
		else if (Rongine::Input::isKeyPressed(Rongine::Key::K))
			m_squarePosition.y -= m_cameraMoveSpeed * ts;

		Rongine::RenderCommand::setColor({ 0.1f, 0.1f, 0.1f, 1 });
		Rongine::RenderCommand::clear();

		m_camera.setPosition(m_cameraPosition);
		m_camera.setRotation(m_cameraRotation);

		Rongine::Renderer::beginScene(m_camera);

		glm::mat4 scale=glm::scale(glm::mat4(1.0f), glm::vec3(0.1f));

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
				glm::mat4 transform = glm::translate(glm::mat4(1.0f) ,pos)*scale;
				Rongine::Renderer::submit(std::dynamic_pointer_cast<Rongine::OpenGLShader>(m_flatColorShader), m_squareVA, transform);
			}
		}
		Rongine::Renderer::submit(m_flatColorShader, m_squareVA,scale);
		//////////////////end

		m_texture->bind();
		Rongine::Renderer::submit(m_textureShader, m_vertexArray, glm::scale(glm::mat4(1.0f), glm::vec3(1.5f)));

		//Rongine::Renderer::submit(m_shader, m_vertexArray);

		Rongine::Renderer::endScene();
	}

	void onEvent(Rongine::Event& event)
	{
		//RONG_CLIENT_TRACE(event.toString());
		if (event.getEventType() == Rongine::EventType::KeyPressed)
		{
			Rongine::KeyPressedEvent& e = (Rongine::KeyPressedEvent&)event;
			if (e.getKeyCode() == Rongine::Key::A)
			{
				RONG_CLIENT_TRACE("A key event received(EVENT)!");
			}
		}
	}

	void onImGuiRender()
	{
		ImGui::Begin("Setting");
		ImGui::ColorEdit3("Square Color", glm::value_ptr(m_squareColor));
		ImGui::End();
	}

private:
	Rongine::Ref<Rongine::Shader> m_shader;
	Rongine::Ref<Rongine::VertexArray> m_vertexArray;

	Rongine::Ref<Rongine::Shader> m_flatColorShader,m_textureShader;
	Rongine::Ref<Rongine::VertexArray> m_squareVA;

	Rongine::Ref<Rongine::Texture2D> m_texture;

	Rongine::OrthographicCamera m_camera;

	glm::vec3 m_cameraPosition = { 0.0f,0.0f,0.0f };
	float m_cameraMoveSpeed = 3.0f;
	float m_cameraRotation = 0.0f;
	float m_cameraRotationSpeed = 90.0f;

	glm::vec3 m_squareColor = { 0.2f, 0.3f, 0.8f };
	glm::vec3 m_squarePosition = {0.0f,0.0f,0.0f};
};

class Sandbox :public Rongine::Application {
public:
	Sandbox() {
		pushLayer(new ExampleLayer());
		pushOverLayer(new Rongine::ImGuiLayer());
	}
	~Sandbox() {

	}
};

Rongine::Application* Rongine::createApplication()
{
	return new Sandbox;
}