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

		float vertex[3 * 7] = {
			-0.5f, -0.5f, 0.0f,0.0f,0.0f,1.0f,1.0f,
			 0.5f, -0.5f, 0.0f,0.0f,1.0f,0.0f,1.0f,
			 0.0f,  0.5f, 0.0f,1.0f,0.0f,1.0f,1.0f
		};

		std::shared_ptr<Rongine::VertexBuffer> vertexBuffer;
		vertexBuffer.reset(Rongine::VertexBuffer::create(vertex, sizeof(vertex)));

		Rongine::BufferLayout layout = {
			{Rongine::ShaderDataType::Float3,"a_Positon"},
			{Rongine::ShaderDataType::Float4,"a_Color"}
		};
		vertexBuffer->setLayout(layout);
		m_vertexArray->addVertexBuffer(vertexBuffer);


		uint32_t indices[3] = { 0,1,2 };

		std::shared_ptr<Rongine::IndexBuffer> indexBuffer;
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

		std::shared_ptr<Rongine::VertexBuffer> squareVB;
		squareVB.reset(Rongine::VertexBuffer::create(squareVertices, sizeof(squareVertices)));

		squareVB->setLayout({
			{ Rongine::ShaderDataType::Float3, "a_Position" }
			});
		m_squareVA->addVertexBuffer(squareVB);


		uint32_t squareIndices[6] = { 0, 1, 2, 2, 3, 0 };

		std::shared_ptr<Rongine::IndexBuffer> squareIB;
		squareIB.reset(Rongine::IndexBuffer::create(squareIndices, sizeof(squareIndices) / sizeof(uint32_t)));
		m_squareVA->setIndexBuffer(squareIB);

		//////////////////////////////////end

		std::string vertexSrc = R"(
			#version 330 core
			
			layout(location = 0) in vec3 a_Position;
			layout(location = 1) in vec4 a_Color;

			uniform mat4 u_ViewProjection;
			uniform mat4 u_Transform;

			out vec3 v_Position;
			out vec4 v_Color;

			void main()
			{
				v_Position = a_Position;
				v_Color=a_Color;
				gl_Position = u_ViewProjection*u_Transform*vec4(a_Position,1.0f);	
			}
		)";

		std::string fragmentSrc = R"(
			#version 330 core
			
			layout(location = 0) out vec4 color;

			in vec3 v_Position;
			in vec4 v_Color;

			void main()
			{
				color = v_Color;
			}
		)";

		m_shader.reset(Rongine::Shader::create(vertexSrc, fragmentSrc));

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

		std::shared_ptr<Rongine::OpenGLShader> shader_ptr = std::dynamic_pointer_cast<Rongine::OpenGLShader>(m_flatColorShader);
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

		Rongine::Renderer::submit(m_shader, m_vertexArray);

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
	std::shared_ptr<Rongine::Shader> m_shader;
	std::shared_ptr<Rongine::VertexArray> m_vertexArray;

	std::shared_ptr<Rongine::Shader> m_flatColorShader;
	std::shared_ptr<Rongine::VertexArray> m_squareVA;

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